/** @file yield.c
 *  @brief Contains yield interrupt handler and helper functions for
 *  	   installation
 *
 */

#include <asm.h>				/* outb() */
#include <logger.h>				/* log_warn() */
#include <scheduler.h>			/* yield_execution() */
#include <interrupt_defines.h>	/* INT_CTL_PORT, INT_ACK_CURRENT */

int
yield( int tid )
{
    /* Acknowledge interrupt immediately */
    outb(INT_CTL_PORT, INT_ACK_CURRENT);

	tcb_t *tcb = find_tcb(tid);
	if (!tcb) {
		log_warn("Trying to yield to non-existent thread with tid %d", tid);
		return -1;
	}

	return yield_execution(RUNNABLE, tcb, NULL, NULL);
}
