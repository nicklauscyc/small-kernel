/** @file task_vanish.c
 *  @brief Implements fake task_vanish()
 */
#include <logger.h>                /* log() */
#include <x86/asm.h>               /* outb() */
#include <timer_driver.h>		   /* get_total_ticks() */
#include <x86/interrupt_defines.h> /* INT_CTL_PORT, INT_ACK_CURRENT */
#include <lib_life_cycle/life_cycle.h> /* _set_status(), _vanish() */
#include <lib_thread_management/thread_management.h> /* _deschedule() */

/** @brief Fake implementation which only sets status of task and calls vanish
 *         on the current thread
 *
 *  @param status Status to set task threads to
 *  @return Void.
 */
void
task_vanish( int status )
{
	/* Acknowledge interrupt immediately */
	outb(INT_CTL_PORT, INT_ACK_CURRENT);
	log_info("call task_vanish");

	// Does not return
	// TODO is there more to this design?
	_set_status(status);
	_vanish();
}
