/** @file set_status.c
 *  @brief Implements fake set_status()
 */
#include <x86/asm.h>   /* outb() */
#include <x86/interrupt_defines.h> /* INT_CTL_PORT, INT_ACK_CURRENT */
#include <logger.h>    /* log() */
#include <timer_driver.h>		/* get_total_ticks() */
#include <scheduler.h> /* get_running_thread() */
#include <task_manager.h> /* set_task_exit_status() */

/** @brief Sets status of a task
 *
 *  @param status Status to set to
 *  @return Void.
 */
void
_set_status( int status )
{
	log_info("_set_status(): "
			 "status: %d", status);

	set_task_exit_status(status);
}

/** @brief Sets status of a task
 *
 *  @param status Status to set to
 *  @return Void.
 */
void
set_status( int status )
{
	/* Acknowledge interrupt immediately */
	outb(INT_CTL_PORT, INT_ACK_CURRENT);
	_set_status(status);
}
