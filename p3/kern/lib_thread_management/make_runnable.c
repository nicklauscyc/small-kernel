/** @file make_runnable.c
 *  @brief Contains make_runnable interrupt handler and helper functions for
 *  	   installation
 */

#include <asm.h>				/* outb() */
#include <scheduler.h>			/* make_thread_runnable() */
#include <interrupt_defines.h>	/* INT_CTL_PORT, INT_ACK_CURRENT */
#include <task_manager.h>       /* get_tcb_status() */
#include <logger.h>

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
	if (!tcbp || get_tcb_status(tcbp) != DESCHEDULED)
		return -1;

	return make_thread_runnable(tcbp);
}
