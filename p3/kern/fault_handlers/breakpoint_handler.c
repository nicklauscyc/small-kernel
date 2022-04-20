/** @file breakpoint_handler.c
 *  @brief Functions for handling breakpoint traps
 */
#include <seg.h>	/* SEGSEL_KERNEL_CS */
#include <assert.h> /* panic() */
#include <panic_thread.h> /* panic_thread() */

void
breakpoint_handler( int eip, int cs )
{
	if (cs == SEGSEL_KERNEL_CS) {
		panic("[Kernel mode] Breakpoint encountered at 0x%x."
				"Please contact kernel developers.", eip);
	}
	/* TODO: acknowledge signal and call user handler  */

	panic_thread("Unhandled breakpoint fault encountered before 0x%x", eip);
}
