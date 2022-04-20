/** @file vanish.c
 *  @brief Implements vanish()
 */
#include <x86/asm.h>   /* outb() */
#include <x86/interrupt_defines.h> /* INT_CTL_PORT, INT_ACK_CURRENT */
#include <logger.h>    /* log() */
#include <timer_driver.h>		/* get_total_ticks() */
#include <scheduler.h>			/* yield_execution() */

void
_vanish( void )
{
	affirm(yield_execution(DEAD, NULL, NULL, NULL) == 0);
}


void
vanish( void )
{
	/* Acknowledge interrupt immediately */
	outb(INT_CTL_PORT, INT_ACK_CURRENT);
	log_info("call vanish");
	_vanish();
}
