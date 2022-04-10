/** @file segment_not_present_handler.c
 *  @brief Functions for segment not present faults
 */
#include <seg.h>	/* SEGSEL_KERNEL_CS */
#include <assert.h> /* panic() */
#include <simics.h>

void
segment_not_present_handler( int error_code, int cs, int eip )
{
	if (cs == SEGSEL_KERNEL_CS) {
		lprintf("[Kernel mode] Segment not present fault encountered error at "
		        "0x%x.", eip);
	}
	/* TODO: acknowledge signal and call user handler  */

	lprintf("[User mode] Segment not present fault encountered at 0x%x", eip);
}
