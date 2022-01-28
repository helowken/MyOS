#include "syslib.h"

int sysIrqCtl(int req, int irqVec, int policy, int *hookId) {
	Message msg;
	int s;

	msg.m_type = SYS_IRQCTL;
	msg.IRQ_REQUEST = req;
	msg.IRQ_VECTOR = irqVec;
	msg.IRQ_POLICY = policy;
	msg.IRQ_HOOK_ID = *hookId;

	s = taskCall(SYSTASK, SYS_IRQCTL, &msg);
	if (req == IRQ_SET_POLICY)
	  *hookId = msg.IRQ_HOOK_ID;
	return s;
}
