#include "../system.h"

int doUnused(Message *msg) {
	kprintf("SYSTEM: got unused request %d from %d", msg->m_type, msg->m_source);
	return EBADREQUEST;		/* Illegal message type. */
}
