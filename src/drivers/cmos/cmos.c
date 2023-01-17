#include "../drivers.h"
#include "sys/ioc_cmos.h"
#include "time.h"
#include "ibm/cmos.h"
#include "ibm/bios.h"

static void reply(int code, int replyee, int pNum, int status) {
	Message msg;
	int s;

	msg.m_type = code;			/* TASK_REPLY or REVIVE */
	msg.REP_STATUS = status;	/* Result of device operation */
	msg.REP_PROC_NR = pNum;		/* Which user made the request */
	if ((s = send(replyee, &msg)) != OK)
	  panic("CMOS", "sending reply failed", s);
}

static int readRegister(int regAddr) {
/* Read a single CMOS register value. */
	int r = 0;

	sysOutb(RTC_INDEX, regAddr);
	sysInb(RTC_IO, &r);
	return r;
}

static int bcdToDec(int v) {
	return ((v >> 4) & 0x0F) * 10 + (v & 0x0F);
}

static int getCmosTime(struct tm *t, int y2kFlag) {
/* Update the structure pointed to by 't' with the current time as read
 * from CMOS RAM of the RTC. If necessary, the time is converted into a
 * binary format before being stored in the structure.
 */
	int sec, n;
	clock_t t0, t1;

	/* Start a timer to keep us from getting stuck on a dead clock. */
	getUptime(&t0);
	do {
		sec = -1;
		n = 0;
		do {
			getUptime(&t1);
			if (t1 - t0 > 5 * HZ) {
				printf("readClock: CMOS clock appears dead\n");
				return 1;
			}

			/* Clock update in progress? */
			if (readRegister(RTC_REG_A) & RTC_A_UIP)
			  continue;

			t->tm_sec = readRegister(RTC_SEC);
			if (t->tm_sec != sec) {
				/* Seconds changed. First from -1, then because the
				 * clock ticked, which is what we're waiting for to
				 * get a precise reading.
				 */
				sec = t->tm_sec;
				++n;
			}
		} while (n < 2);

		/* Read the other registers. */
		t->tm_min = readRegister(RTC_MIN);
		t->tm_hour = readRegister(RTC_HOUR);
		t->tm_mday = readRegister(RTC_MDAY);
		t->tm_mon = readRegister(RTC_MONTH);
		t->tm_year = readRegister(RTC_YEAR);

		/* Time stable? */
	} while (readRegister(RTC_SEC) != t->tm_sec ||
			readRegister(RTC_MIN) != t->tm_min ||
			readRegister(RTC_HOUR) != t->tm_hour ||
			readRegister(RTC_MDAY) != t->tm_mday ||
			readRegister(RTC_MONTH) != t->tm_mon ||
			readRegister(RTC_YEAR) != t->tm_year);

	if ((readRegister(RTC_REG_B) & RTC_B_DM_BCD) == 0) {
		/* Convert BCD to binary (default RTC mode). */
		t->tm_year = bcdToDec(t->tm_year);
		t->tm_mon = bcdToDec(t->tm_mon);
		t->tm_mday = bcdToDec(t->tm_mday);
		t->tm_hour = bcdToDec(t->tm_hour);
		t->tm_min = bcdToDec(t->tm_min);
		t->tm_sec = bcdToDec(t->tm_sec);
	}
	--t->tm_mon;	/* Counts from 0. */

	/* Correct the year, good until 2080. */
	if (t->tm_year < 80) 
	  t->tm_year += 100;

	if (y2kFlag) {
		/* Clock with Y2K bug, interpret 1980 as 2000, good until 2020. */
		if (t->tm_year < 100)
		  t->tm_year += 20;
	}
	return 0;
}

static int getTime(int who, int y2kFlag, vir_bytes dstTime) {
	unsigned char machineId, cmosState;
	struct tm time1;

	/* First obtain the machine ID to see if we can read the CMOS clock. Only
	 * for PS_386 and PC_AT this is possible. Otherwise, return an error.
	 */
	sysVirCopy(SELF, BIOS_SEG, (vir_bytes) MACHINE_ID_ADDR,
			SELF, D, (vir_bytes) &machineId, MACHINE_ID_SIZE);
	if (machineId != PS_386_MACHINE && machineId != PC_AT_MACHINE) {
		printf("IS: Machine ID unknown. ID byte = %02x.\n", machineId);
		return EFAULT;
	}

	/* Now check the CMOS' state to see if we can read a proper time from it.
	 * If the state is crappy, return an error.
	 */
	cmosState = readRegister(CMOS_STATUS);
	if (cmosState & (CS_LOST_POWER | CS_BAD_CHKSUM | CS_BAD_TIME)) {
		printf("IS: CMOS RAM error(s) found. State = 0x%02x\n", cmosState);
		if (cmosState & CS_LOST_POWER)
		  printf("IS: RTC lost power. Reset CMOS RAM with SETUP.");
		if (cmosState & CS_BAD_CHKSUM)
		  printf("IS: CMOS RAM checksum is bad. Run SETUP.");
		if (cmosState & CS_BAD_TIME)
		  printf("IS: Time invalid in CMOS RAM. Reset clock.");
		return EFAULT;
	}

	/* Everything seems to be OK. Read the CMOS real time clock and copy the
	 * result back to the caller.
	 */
	if (getCmosTime(&time1, y2kFlag) != 0)
	  return EFAULT;
	sysDataCopy(SELF, (vir_bytes) &time1,
				who, dstTime, sizeof(struct tm));
	return OK;
}

void main() {
	Message msg;
	int suspended = NONE;
	int y2kFlag;
	int result;
	int s;

	while (TRUE) {
		/* Get work. */
		if ((s = receive(ANY, &msg)) != OK)
		  panic("CMOS", "attempt to receive work failed", s);

		switch (msg.m_type) {
			case DEV_OPEN:
			case DEV_CLOSE:
			case CANCEL:
				reply(TASK_REPLY, msg.m_source, msg.PROC_NR, OK);
				break;
			case DEV_IOCTL:
				/* Probably best to SUSPEND the caller, CMOS I/O has nasty timeouts.
				 * This way we don't block the rest of the system. First check if
				 * another process is already suspended. We cannot handle multiple
				 * requests at a time.
				 */
				if (suspended != NONE) {
					reply(TASK_REPLY, msg.m_source, msg.PROC_NR, EBUSY);
					break;
				}
				suspended = msg.PROC_NR;
				reply(TASK_REPLY, msg.m_source, msg.PROC_NR, SUSPEND);

				switch (msg.REQUEST) {
					case CIOC_GET_TIME:		/* Get CMOS time */
					case CIOC_GET_TIME_Y2K:
						y2kFlag = (msg.REQUEST == CIOC_GET_TIME) ? 0 : 1;
						result = getTime(msg.PROC_NR, y2kFlag, (vir_bytes) msg.ADDRESS);
						break;
					case CIOC_SET_TIME:
					case CIOC_SET_TIME_Y2K:
					default:				/* Unsupported ioctl */
						result = ENOSYS;
				}
				/* Request completed. Tell the caller to check our status. */
				notify(msg.m_source);
				break;
			case DEV_STATUS:
				/* The FS calls back to get our status. Revive the suspended
				 * processes and return the status of reading the CMOS.
				 */
				if (suspended == NONE)
				  reply(DEV_NO_STATUS, msg.m_source, NONE, OK);
				else
				  reply(DEV_REVIVE, msg.m_source, suspended, result);
				suspended = NONE;
				break;
			case SYN_ALARM:		/* Shouldn't happen */
			case SYS_SIG:		/* Ignore system events */
				continue;
			default:
				reply(TASK_REPLY, msg.m_source, msg.PROC_NR, EINVAL);
		}
	}
}
