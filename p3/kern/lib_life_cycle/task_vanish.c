/** @file task_vanish.c
 *  @brief Implements fake task_vanish()
 */
#include <x86/asm.h>   /* outb() */
#include <x86/interrupt_defines.h> /* INT_CTL_PORT, INT_ACK_CURRENT */
#include <logger.h>    /* log() */
#include <timer_driver.h>		/* get_total_ticks() */
#include <lib_thread_management/thread_management.h> /* _deschedule() */

#include <lib_life_cycle/life_cycle.h>

void
task_vanish( int status )
{
	/* Acknowledge interrupt immediately */
	outb(INT_CTL_PORT, INT_ACK_CURRENT);
	log_info("call task_vanish");

	// Does not return
	// TODO is there more to this design?
	_vanish();

}
