/** @file general_protection_handler.c
 *  @brief Functions for alignment check faults
 */
#include <seg.h>	/* SEGSEL_KERNEL_CS */
#include <assert.h> /* panic() */
#include <simics.h>

void
general_protection_handler( int error_code, int eip, int cs )
{
	affirm_msg(error_code == 0, "General protection fault while loading a "
	           "segment descriptor");

	if (cs == SEGSEL_KERNEL_CS) {
		lprintf("[Kernel mode] General protection fault encountered error at "
		        "0x%x.", eip);
	}
	/* TODO: acknowledge signal and call user handler  */

	lprintf("[User mode] General protection fault encountered at 0x%x", eip);
}
