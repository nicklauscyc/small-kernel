/** @file deschedule.c
 *  @brief Contains deschedule interrupt handler and helper functions for
 *  	   installation
 *
 */

#include <asm.h>				/* outb() */
#include <stddef.h>				/* NULL */
#include <scheduler.h>			/* yield_execution() */
#include <memory_manager.h>		/* is_user_pointer_valid() */
#include <install_handler.h>	/* install_handler_in_idt() */
#include <interrupt_defines.h>	/* INT_CTL_PORT, INT_ACK_CURRENT */

/** @brief Deschedules currently running thread if *reject == 0
 *		   Atomic w.r.t make_runnable
 *
 *	@param reject Pointer to integer describing whether to deschedule
 *	@return 0 on success, negative value on failure */
int
deschedule( int *reject )
{
    /* Acknowledge interrupt immediately */
    outb(INT_CTL_PORT, INT_ACK_CURRENT);

	if (!is_user_pointer_valid(reject))
		return -1;

	if (*reject == 0)
		return yield_execution(NULL, DESCHEDULED, -1);
	return 0;
}
