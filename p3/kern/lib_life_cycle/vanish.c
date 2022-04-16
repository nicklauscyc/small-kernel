/** @file vanish.c
 *  @brief Implements vanish()
 */
#include <x86/asm.h>   /* outb() */
#include <x86/interrupt_defines.h> /* INT_CTL_PORT, INT_ACK_CURRENT */
#include <logger.h>    /* log() */
#include <timer_driver.h>		/* get_total_ticks() */

void
_vanish( void )
{
	log_info("call _vanish()");
	while(1) {
		continue;
	}
}

void
vanish( void )
{
	/* Acknowledge interrupt immediately */
	outb(INT_CTL_PORT, INT_ACK_CURRENT);
	log_info("call vanish");
	_vanish();
}



