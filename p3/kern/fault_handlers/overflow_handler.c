/** @file overflow_handler.c
 *  @brief Functions for handling overflow traps
 */
#include <seg.h>	/* SEGSEL_KERNEL_CS */
#include <assert.h> /* panic() */
#include <simics.h>

void
overflow_handler( int eip, int cs )
{
	if (cs == SEGSEL_KERNEL_CS) {
		lprintf("[Kernel mode] Overflow encountered at 0x%x.", eip);
	}
	/* TODO: acknowledge signal and call user handler  */

	lprintf("[User mode] Unhandled overflow fault encountered at 0x%x", eip);
}
