/** @file float_handler.c
 *  @brief Functions for handling devince not avaiable faults
 */
#include <seg.h>	/* SEGSEL_KERNEL_CS */
#include <assert.h> /* panic() */

void
float_handler( int cs, int eip )
{
	if (cs == SEGSEL_KERNEL_CS) {
		panic("[Kernel mode] Floating point operation encountered at 0x%x."
				"Please contact kernel developers.", eip);
	}

	/* acknowledge signal and call user handler?
	 * OR
	 * acknowledge signal and just kill user thread? */

	panic("Unhandled devince not available fault (due to floating-point op)"
			" at instruction 0x%x", eip);
}
