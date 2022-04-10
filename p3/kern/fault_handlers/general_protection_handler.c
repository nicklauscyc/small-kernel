/** @file general_protection_handler.c
 *  @brief Functions for alignment check faults
 */
#include <seg.h>	/* SEGSEL_KERNEL_CS */
#include <assert.h> /* panic() */
#include <simics.h>

void
general_protection_handler( int error_code, int cs, int eip )
{
	if (cs == SEGSEL_KERNEL_CS) {
		lprintf("[Kernel mode] General protection fault encountered error at "
		        "0x%x.", eip);
	}
	/* TODO: acknowledge signal and call user handler  */

	lprintf("[User mode] General protection fault encountered at 0x%x", eip);
}
