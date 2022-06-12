#include "../drivers.h"
#include "../libdriver/driver.h"
#include "minix/type.h"
#include "minix/const.h"
#include "minix/com.h"
#include "sys/types.h"
#include "minix/ipc.h"

#define LOG_SIZE	(50*1024)
#define SUSPENDABLE		1

typedef struct {
	char log_buffer[LOG_SIZE];
	int log_size;	/* No. of bytes in log buffer */
	int log_read;	/* Read mark */
	int log_write;	/* Write mark */
#if SUSPENDABLE
	int log_proc_nr;
	int log_source;
	int log_io_size;
	int log_revive_alerted;
	int log_status;	/* Proc that is blocking on read */
	vir_bytes log_user_vir;
#endif
	int log_selected;
	int log_select_proc;
	int log_select_alerted;
	int log_select_ready_ops;
} LogDevice;

void kputc(int c);
int doNewKernelMsgs(Message *msg);
int doDiagnostics(Message *msg);
void logAppend(char *buf, int len);
