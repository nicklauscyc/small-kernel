/** @file overflow_handler.c
 *  @brief Functions for handling overflow traps
 */
#include <seg.h>	/* SEGSEL_KERNEL_CS */
#include <assert.h> /* panic() */
#include <panic_thread.h> /* panic_thread() */

void
overflow_handler( int eip, int cs )
{
	if (cs == SEGSEL_KERNEL_CS) {
		panic("[Kernel mode] Overflow encountered at 0x%x.", eip);
	}
	/* TODO: acknowledge signal and call user handler  */

	panic_thread("Unhandled overflow fault encountered at 0x%x", eip);
}
