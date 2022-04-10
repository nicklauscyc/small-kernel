/** @file alignment_check_handler.c
 *  @brief Functions for alignment check faults
 */
#include <seg.h>	/* SEGSEL_KERNEL_CS */
#include <assert.h> /* panic() */
#include <simics.h>

void
alignment_check_handler( int error_code, int cs, int eip )
{
	assert(error_code == 0);
	if (cs == SEGSEL_KERNEL_CS) {
		lprintf("[Kernel mode] Alignment check fault  encountered error at "
		        "0x%x.", eip);
	}
	/* TODO: acknowledge signal and call user handler  */

	lprintf("[User mode] Alignment check fault encountered at 0x%x", eip);
}
