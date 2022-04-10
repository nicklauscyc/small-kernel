/** @file float_handler.c
 *  @brief Functions for handling faults
 */
#include <seg.h>	/* SEGSEL_KERNEL_CS */
#include <assert.h> /* panic() */

void
float_handler( int cs, int eip )
{
	if (cs == SEGSEL_KERNEL_CS) {
		panic("[Kernel mode] floating point device not available  encountered "
		      "at 0x%x."
				"Please contact kernel developers.", eip);
	}
	/* TODO: acknowledge signal and call user handler  */

	panic("Unhandled floating point device not available encountered at 0x%x",
	      eip);

}
