/** @file bound_handler.c
 *  @brief Functions for handling bound range faults
 */
#include <seg.h>	/* SEGSEL_KERNEL_CS */
#include <assert.h> /* panic() */

void
bound_handler( int eip, int cs)
{
	if (cs == SEGSEL_KERNEL_CS) {
		panic("[Kernel mode] Bound-range-exceeded fault encountered at 0x%x."
				"Please contact kernel developers.", eip);
	}
	/* TODO: acknowledge signal and call user handler  */

	panic("Unhandled bound-range-exceeded fault encountered at 0x%x", eip);
}
