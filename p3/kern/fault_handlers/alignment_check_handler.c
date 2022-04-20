/** @file alignment_check_handler.c
 *  @brief Functions for alignment check faults
 */
#include <seg.h>	/* SEGSEL_KERNEL_CS */
#include <assert.h> /* panic() */
#include <panic_thread.h> /* panic_thread() */

void
alignment_check_handler( int error_code, int eip, int cs )
{
	assert(error_code == 0);
	if (cs == SEGSEL_KERNEL_CS) {
		panic("[Kernel mode] Alignment check fault  encountered error at "
		        "0x%x.", eip);
	}
	/* TODO: acknowledge signal and call user handler  */

	panic_thread("Unhandled alignment check fault encountered at 0x%x", eip);
}
