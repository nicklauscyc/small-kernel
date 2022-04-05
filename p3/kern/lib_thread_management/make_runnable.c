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
	add_to_runnable_queue(tcbp);

	return 0;
}

/** @brief Installs the yield() interrupt handler
 */
int
install_make_runnable_helper(int idt_entry, asm_wrapper_t *asm_wrapper)
{
	if (!asm_wrapper) {
		return -1;
	}
	int res = install_handler_in_idt(idt_entry, asm_wrapper, DPL_3);
	return res;
}
