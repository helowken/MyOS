#include "../system.h"

static int genericHandler(IrqHook *hook) {
/* This function handles hardware interrupt in a simple and generic way. All
 * interrupts are transformed into messages to a driver. The IRQ line will be
 * reenabled if the policy says so.
 */

	/* As a side-effect, the interrupt handler gathers random information by
	 * timestamping the interrupt events. This is used for /dev/random.
	 */
	getRandomness(hook->irq);

	/* Add a bit for this interrupt to the process' pending interrupts. When
	 * sending the notification message, this bit map will be magically set
	 * as an argument.
	 */
	priv(procAddr(hook->pNum))->s_int_pending |= 1 << hook->notifyId;

	/* Build notification message and return. */
	lockNotify(HARDWARE, hook->pNum);
	return hook->policy & IRQ_REENABLE;
}

int doIrqCtl(register Message *msg) {
	int irqVec;
	int irqHookId;
	int notifyId;
	int r = OK;
	IrqHook *hookPtr;
	
	irqHookId = (unsigned) msg->IRQ_HOOK_ID - 1;
	irqVec = (unsigned) msg->IRQ_VECTOR;

	switch (msg->IRQ_REQUEST) {
		/* Enable or disable IRQs. */
		case IRQ_ENABLE:
		case IRQ_DISABLE:
			if (irqHookId >= NR_IRQ_HOOKS ||
				irqHooks[irqHookId].pNum == NONE)
			  return EINVAL;
			if (irqHooks[irqHookId].pNum != msg->m_source)
			  return EPERM;
			if (msg->IRQ_REQUEST == IRQ_ENABLE)
			  enableIrq(&irqHooks[irqHookId]);
			else
			  disableIrq(&irqHooks[irqHookId]);
			break;

		/* Control IRQ policies. Set a policy and needed details 
		 * in the IRQ table. This policy is used by a generic 
		 * function to handle hardware interrupts.
		 */
		case IRQ_SET_POLICY:
			/* Check if IRQ line is acceptable. */
			if (irqVec < 0 || irqVec >= NR_IRQ_VECTORS)
			  return EINVAL;
			
			/* Find a free IRQ hook for this mapping. */
			hookPtr = NULL;
			for (irqHookId = 0; irqHookId < NR_IRQ_HOOKS; ++irqHookId) {
				if (irqHooks[irqHookId].pNum == NONE) {
					hookPtr = &irqHooks[irqHookId];		/* Free hook */
					break;
				}
			}
			if (hookPtr == NULL)
			  return ENOSPC;

			/* When setting a policy, the caller must provide an identifier 
			 * that is returned on the notification message if a interrupt
			 * occurs.
			 */
			notifyId = (unsigned) msg->IRQ_HOOK_ID;
			if (notifyId > CHAR_BIT * sizeof(irq_id_t) - 1)
			  return EINVAL;

			/* Install the handler. */
			hookPtr->pNum = msg->m_source;		/* Process to notify */
			hookPtr->notifyId = notifyId;		/* Identifier to pass */
			hookPtr->policy = msg->IRQ_POLICY;	/* Policy for interrupts */
			putIrqHandler(hookPtr, irqVec, genericHandler);

			/* Return index of the IRQ hook in use. */
			msg->IRQ_HOOK_ID = irqHookId + 1;
			break;

		case IRQ_RM_POLICY:
			if (irqHookId >= NR_IRQ_HOOKS ||
					irqHooks[irqHookId].pNum == NONE)
			  return EINVAL;
			if (msg->m_source != irqHooks[irqHookId].pNum)
			  return EPERM;
			/* Remove the handler and return. */
			removeIrqHandler(&irqHooks[irqHookId]);
			break;

		default:
			r = EINVAL;
	}
	return r;
}





