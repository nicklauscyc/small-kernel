/** @file vanish.c
 *  @brief Implements vanish()
 */
#include <x86/asm.h>   /* outb() */
#include <x86/interrupt_defines.h> /* INT_CTL_PORT, INT_ACK_CURRENT */
#include <logger.h>    /* log() */
#include <timer_driver.h>		/* get_total_ticks() */
#include <scheduler.h>			/* yield_execution() */

//static mutex_t hacky_vanish_mux;
//static int hacky_vanish_mux_init = 0;

void
_vanish( void )
{
	log_warn("Vanishing");
	affirm(yield_execution(DEAD, -1, NULL, NULL) == 0);
}


void
vanish( void )
{
	/* Acknowledge interrupt immediately */
	outb(INT_CTL_PORT, INT_ACK_CURRENT);
	log_info("call vanish");
	_vanish();
}



