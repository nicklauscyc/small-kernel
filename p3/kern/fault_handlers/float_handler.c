/** @file float_handler.c
 *  @brief Functions for handling device not available faults
 */
#include <seg.h>	/* SEGSEL_KERNEL_CS */
#include <assert.h> /* panic() */

void
float_handler( int eip, int cs )
{
	if (cs == SEGSEL_KERNEL_CS) {
		panic("[Kernel mode] Floating point operation encountered at 0x%x."
				"Please contact kernel developers.", eip);
	}

	/* acknowledge signal and call user handler?
	 * OR
	 * acknowledge signal and just kill user thread? */

	panic("Unhandled device not available fault (due to floating-point op)"
			" at instruction 0x%x", eip);
}
