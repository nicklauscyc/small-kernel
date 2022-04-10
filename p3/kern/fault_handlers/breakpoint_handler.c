/** @file breakpoint_handler.c
 *  @brief Functions for handling breakpoint traps
 */
#include <seg.h>	/* SEGSEL_KERNEL_CS */
#include <assert.h> /* panic() */

void
breakpoint_handler( int cs, int eip )
{
	if (cs == SEGSEL_KERNEL_CS) {
		panic("[Kernel mode] Breakpoint encountered at 0x%x."
				"Please contact kernel developers.", eip);
	}
	/* TODO: acknowledge signal and call user handler  */

	panic("Unhandled breakpoint fault encountered before 0x%x", eip);
}
