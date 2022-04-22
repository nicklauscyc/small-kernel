/** @file float_handler.c
 *  @brief Functions for handling device not available faults
 */
#include <seg.h>	/* SEGSEL_KERNEL_CS */
#include <assert.h> /* panic() */
#include <panic_thread.h> /* panic_thread() */

void
float_handler( int *ebp )
{
	int eip	= *(ebp + 1);
	int cs	= *(ebp + 2);

	if (cs == SEGSEL_KERNEL_CS) {
		panic("[Kernel mode] Floating point operation encountered at 0x%x."
				"Please contact kernel developers.", eip);
	}

	/* acknowledge signal and call user handler?
	 * OR
	 * acknowledge signal and just kill user thread? */

	panic_thread("Unhandled device not available fault "
			"(due to floating-point op) at instruction 0x%x", eip);
}
