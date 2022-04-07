/** @file yield.c
 *  @brief Contains yield interrupt handler and helper functions for
 *  	   installation
 *
 */

#include <asm.h>				/* outb() */
#include <scheduler.h>			/* yield_execution() */
#include <interrupt_defines.h>	/* INT_CTL_PORT, INT_ACK_CURRENT */

int
yield( int tid )
{
    /* Acknowledge interrupt immediately */
    outb(INT_CTL_PORT, INT_ACK_CURRENT);

	return yield_execution(RUNNABLE, tid, NULL, NULL);
}
