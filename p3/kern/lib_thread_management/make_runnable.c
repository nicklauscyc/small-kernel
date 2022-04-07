/** @file make_runnable.c
 *  @brief Contains make_runnable interrupt handler and helper functions for
 *  	   installation
 *
 */

#include <asm.h>				/* outb() */
#include <stddef.h>				/* NULL */
#include <scheduler.h>			/* yield_execution() */
#include <install_handler.h>	/* install_handler_in_idt() */
#include <interrupt_defines.h>	/* INT_CTL_PORT, INT_ACK_CURRENT */

/** @brief Makes a previously descheduled thread runnable.
 *		   Atomic w.r.t. deschedule.
 *
 *	@param tid Id of descheduled thread to make runnable
 *	@return 0 on success, negative error code if thread pointed to by tid
 *			does not exist or is not descheduled */
int
make_runnable( int tid )
{
    /* Acknowledge interrupt immediately */
    outb(INT_CTL_PORT, INT_ACK_CURRENT);

	tcb_t *tcbp = find_tcb(tid);
	if (!tcbp || tcbp->status != DESCHEDULED)
		return -1;

	/* move to runnable queue and mark as runnable */
	return make_thread_runnable(tid);
}
