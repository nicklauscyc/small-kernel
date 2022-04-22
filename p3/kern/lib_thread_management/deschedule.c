/** @file deschedule.c
 *  @brief Contains deschedule interrupt handler and helper functions for
 *  	   installation
 *
 */

#include <asm.h>				/* outb() */
#include <stddef.h>				/* NULL */
#include <scheduler.h>			/* yield_execution() */
#include <memory_manager.h>		/* is_valid_user_pointer() */
#include <install_handler.h>	/* install_handler_in_idt() */
#include <interrupt_defines.h>	/* INT_CTL_PORT, INT_ACK_CURRENT */
#include <logger.h>

int
_deschedule( int *reject )
{
	log_info("_deschedule(): called!");

	if (!is_valid_user_pointer(reject, READ_ONLY))
		return -1;

	if (*reject == 0)
		return yield_execution(DESCHEDULED, NULL, NULL, NULL);
	return 0;
}

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

	return _deschedule(reject);
}
