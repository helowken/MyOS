#include "log.h"
#include "sys/time.h"
#include "sys/select.h"

#define LOG_DEBUG	0	/* enable/disable debugging */

#define	NR_DEVS		1	/* Number of minor devices */
#define MINOR_KLOG	0	/* /dev/klog */

#define LOG_INC(n, i)	do { (n) = (((n) + (i)) % LOG_SIZE); } while(0)

LogDevice logDevices[NR_DEVS];
static Device logGeom[NR_DEVS];		/* Base and size of devices */
static int currDevIdx = -1;			/* Current device */

extern int deviceCaller;

static char *logName() {
/* Return a name for the current device. */
	static char name[] = "log";
	return name;
}

static Device *logPrepare(int dev) {
/* Prepare for I/O on a device: check if the minor device number is ok. */
	if (dev < 0 || dev >= NR_DEVS)
	  return NIL_DEV;
	currDevIdx = dev;
	return &logGeom[dev];
}

static int logDoOpen(Driver *dp, Message *msg) {
	if (logPrepare(msg->DEVICE) == NIL_DEV)
	  return ENXIO;
	return OK;
}

static int subRead(LogDevice *logDev, int count, int pNum, vir_bytes userVirAddr) {
	char *buf;
	int r;

	if (count > logDev->log_size)
	  count = logDev->log_size;
	if (logDev->log_read + count > LOG_SIZE)
	  count = LOG_SIZE - logDev->log_read;
	
	buf = logDev->log_buffer + logDev->log_read;
	if ((r = sysVirCopy(SELF, D, (vir_bytes) buf, 
				pNum, D, userVirAddr, count)) != OK) 
	  return r;

	LOG_INC(logDev->log_read, count);
	logDev->log_size -= count;

	return count;	
}

static int subWrite(LogDevice *logDev, int count, int pNum, vir_bytes userVirAddr) {
	char *buf;
	int r;

	if (logDev->log_write + count > LOG_SIZE) 
	  count = LOG_SIZE - logDev->log_write;

	buf = logDev->log_buffer + logDev->log_write;

	if (pNum == SELF) {
		memcpy(buf, (char *) userVirAddr, count);
	} else {
		if ((r = sysVirCopy(pNum, D, userVirAddr, 
					SELF, D, (vir_bytes) buf, count)) != OK)
		  return r;
	}
	
	LOG_INC(logDev->log_write, count);
	logDev->log_size += count;

	if (logDev->log_size > LOG_SIZE) {
		/* Discard the overflow part */
		int overflow = logDev->log_size - LOG_SIZE;
		logDev->log_size -= overflow;
		LOG_INC(logDev->log_read, overflow);
	}

	if (logDev->log_size > 0 && 
			logDev->log_proc_nr &&
			!logDev->log_revive_alerted) {
		/* Someone who was suspended on read can now be revived. */
		logDev->log_status = subRead(logDev, logDev->log_io_size, 
					logDev->log_proc_nr, logDev->log_user_vir);
		notify(logDev->log_source);
		logDev->log_revive_alerted = 1;
	}

	if (logDev->log_size > 0)
	  logDev->log_select_ready_ops |= SEL_RD;

	if (logDev->log_size > 0 && 
			logDev->log_selected && 
			!logDev->log_select_alerted) {
		/* Someone(s) who was/were select()ing can now
		 * be awoken. If there was a blocking read (aboved),
		 * this can only happen if the blocking read didn't
		 * swallow all the data (log_size > 0).
		 */
		if (logDev->log_selected & SEL_RD) {
			notify(logDev->log_select_proc);
			logDev->log_select_alerted = 1;
#if LOG_DEBUG
			printf("log notified %d\n", logDev->log_select_proc);
#endif
		}
	}
	return count;
}

static int logTransfer(int pNum, int opCode, off_t position, 
						IOVec *iov, unsigned numReq) {
/* Read or write on the driver's minor devices. */
	unsigned count;
	vir_bytes userVirAddr;
	LogDevice *logDev;
	int accumulatedRead = 0;

	if (currDevIdx < 0 || currDevIdx >= NR_DEVS)
	  return EIO;

	/* Get minor device number and check for /dev/null. */
	logDev = &logDevices[currDevIdx];	

	while (numReq > 0) {
		/* How much to transfer and where to/from. */
		count = iov->iov_size;
		userVirAddr = iov->iov_addr;

		switch (currDevIdx) {
			case MINOR_KLOG:
				if (opCode == DEV_GATHER) {
					if (logDev->log_proc_nr || count < 1) {
						/* There's already someone hanging to read, or 
						 * no real I/O requested.
						 */
						return OK;
					}

					if (!logDev->log_size) {
						if (accumulatedRead)
						  return OK;
						/* No data available; let caller back. */
						logDev->log_proc_nr = pNum;
						logDev->log_io_size = count;
						logDev->log_user_vir = userVirAddr;
						logDev->log_revive_alerted = 0;

						/* DeviceCaller is a global in drivers library. */
						logDev->log_source = deviceCaller;
#if LOG_DEBUG	
						printf("blocked %d (%d)\n", 
								logDev->log_source, logDev->log_proc_nr);
#endif	
						return SUSPEND;
					}
					count = subRead(logDev, count, pNum, userVirAddr);
					if (count < 0)
					  return count;
					accumulatedRead += count;
				} else {
					count = subWrite(logDev, count, pNum, userVirAddr);
					if (count < 0)
					  return count;
				}
				break;
			/* Unknown (illegal) minor device. */
			default:
				return EINVAL;
		}

		/* Book the number of bytes transferred. */
		iov->iov_addr += count;
		if ((iov->iov_size -= count) == 0) {
			++iov;
			--numReq;
		}
	}
	return OK;
}

static void logGeometry(Partition *entry) {
/* Take a page from the fake memory device geometry. */
	entry->heads = 64;
	entry->sectors = 32;
	entry->cylinders = div64u(logGeom[currDevIdx].dv_size, SECTOR_SIZE) /
						(entry->heads * entry->sectors);
}

static void logSignal(Driver *dp, Message *msg) {
	sigset_t sigset = msg->NOTIFY_ARG;
	if (sigismember(&sigset, SIGKMESS))
	  doNewKernelMsgs(msg);
}

static int logCancel(Driver *dp, Message *msg) {
	int d;

	d = msg->TTY_LINE;
	if (d < 0 || d >= NR_DEVS)
	  return EINVAL;
	logDevices[d].log_proc_nr = 0;
	logDevices[d].log_revive_alerted = 0;
	return OK;
}

static void doStatus(Message *msg) {
	int d;
	Message m;

	/* Caller has requested pending status information, which currently
	 * can be pending available select()s, or REVIVE events. One message
	 * is returned for every event, or DEV_NO_STATUS if no (more) events
	 * are to be returned.
	 */

	for (d = 0; d < NR_DEVS; ++d) {
		/* Check for revive callback. */
		if (logDevices[d].log_proc_nr && 
				logDevices[d].log_revive_alerted &&
				logDevices[d].log_source == msg->m_source) {
			m.m_type = DEV_REVIVE;
			m.REP_PROC_NR = logDevices[d].log_proc_nr;
			m.REP_STATUS = logDevices[d].log_status;
			send(msg->m_source, &m);
			logDevices[d].log_proc_nr = 0;
			logDevices[d].log_revive_alerted = 0;
#if LOG_DEBUG
			printf("revived %d with %d bytes\n",
				m.REP_PROC_NR, m.REP_STATUS);
#endif	
			return;
		}

		/* Check for select callback. */
		if (logDevices[d].log_selected && 
				logDevices[d].log_select_alerted &&
				logDevices[d].log_select_proc == msg->m_source) {
			m.m_type = DEV_IO_READY;
			m.DEV_SEL_OPS = logDevices[d].log_select_ready_ops;
			m.DEV_MINOR = d;
#if LOG_DEBUG
			printf("select sending sent\n");
#endif
			send(msg->m_source, &m);
			logDevices[d].log_selected &= ~logDevices[d].log_select_ready_ops;
			logDevices[d].log_select_alerted = 0;
#if LOG_DEBUG
			printf("select send sent\n");
#endif
			return;
		}
	}
	
	/* No event found. */
	m.m_type = DEV_NO_STATUS;
	send(msg->m_source, &m);
}

static int logSelect(Driver *dp, Message *msg) {
	int d, readyOps = 0, ops = 0;
	d = msg->TTY_LINE;
	if (d < 0 || d >= NR_DEVS) {
#if LOG_DEBUG
		printf("line %d? EINVAL\n", d);
#endif
		return EINVAL;
	}

	ops = msg->PROC_NR & (SEL_RD | SEL_WR | SEL_ERR);

	/* Read blocks when there is no log. */
	if ((msg->PROC_NR & SEL_RD) && logDevices[d].log_size > 0) {
#if LOG_DEBUG
		pritnf("log can read; size %d\n", logDevices[d].log_size);
#endif
		readyOps |= SEL_RD;		/* Writes never block */
	}

	/* Write never blocks. */
	if (msg->PROC_NR & SEL_WR)
	  readyOps |= SEL_WR;

	/* Enable select callback if no operations were ready to go,
	 * but operations were requested, and notify was enabled.
	 */
	if ((msg->PROC_NR & SEL_NOTIFY) && ops && !readyOps) {
		logDevices[d].log_selected |= ops;
		logDevices[d].log_select_proc = msg->m_source;
#if LOG_DEBUG
		printf("log setting selector. \n");
#endif
	}

#if LOG_DEBUG
	printf("log returning ops %d\n", readyOps);
#endif

	return readyOps;
}

static int logOther(Driver *dp, Message *msg) {
	int r;

	/* This function gets messages that the generic driver doesn't
	 * understand.
	 */
	switch (msg->m_type) {
		case DIAGNOSTICS: 
			r = doDiagnostics(msg);
			break;
		case DEV_STATUS:
			doStatus(msg);
			r = EDONTREPLY;
			break;
		default:
			r = EINVAL;
			break;
	}
	return r;
}

void logAppend(char *buf, int len) {
	 int w = 0, skip = 0;

	 if (len < 1)
	   return;
	 if (len > LOG_SIZE)
	   skip = len - LOG_SIZE;
	 len -= skip;
	 buf += skip;
	 w = subWrite(&logDevices[0], len, SELF, (vir_bytes) buf);

	 if (w > 0 && w < len)
	   subWrite(&logDevices[0], len - w, SELF, (vir_bytes) buf + w);
}

static Driver logDriverTable = {
	logName,		/* Current device's name */
	logDoOpen,		/* Open or mount */
	doNop,			/* Nothing on a close */
	doNop,			/* Ioctl nop */
	logPrepare,		/* Prepare for I/O on a given minor device */
	logTransfer,	/* Do the I/O */
	nopCleanup,		/* No need to clean up */
	logGeometry,	/* Geometry */
	logSignal,		/* Handle system signal */
	nopAlarm,		/* No alarm */
	logCancel,		/* CANCEL request */
	logSelect,		/* DEV_SELECT request */
	logOther,		/* Unrecognized messages */
	NULL			/* HW int */
};


int main() {
	int i;

	for (i = 0; i < NR_DEVS; ++i) {
		logGeom[i].dv_size = cvul64(LOG_SIZE);
		logGeom[i].dv_base = cvul64((long) logDevices[i].log_buffer);
		logDevices[i].log_size =
			logDevices[i].log_read = 
			logDevices[i].log_write = 
			logDevices[i].log_select_alerted = 
			logDevices[i].log_selected = 
			logDevices[i].log_select_ready_ops = 0;
		logDevices[i].log_proc_nr = 0;
		logDevices[i].log_revive_alerted = 0;
	}
	driverTask(&logDriverTable);
	return OK;
}




















