/** @file yield.c
 *  @brief Contains yield interrupt handler
 *
 */

#include <asm.h>				/* outb() */
#include <logger.h>				/* log_warn() */
#include <scheduler.h>			/* yield_execution() */
#include <interrupt_defines.h>	/* INT_CTL_PORT, INT_ACK_CURRENT */

int
yield( int tid )
{
	log_info("yield(): called with argument tid:%d!", tid);
    /* Acknowledge interrupt immediately */
    outb(INT_CTL_PORT, INT_ACK_CURRENT);

	/* Yield to any thread of schedulers chosing if tid is -1 */
	if (tid == -1) {
		return yield_execution(RUNNABLE, NULL, NULL, NULL);
	}
	/* Else see if given tid is valid */
	tcb_t *tcb = find_tcb(tid);
	if (!tcb) {
		log_info("Trying to yield to non-existent thread with tid %d", tid);
		return -1;
	}
	return yield_execution(RUNNABLE, tcb, NULL, NULL);
}
