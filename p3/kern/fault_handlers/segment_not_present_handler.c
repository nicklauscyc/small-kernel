/** @file segment_not_present_handler.c
 *  @brief Functions for handling segment not present faults
 */
#include <seg.h>	/* SEGSEL_KERNEL_CS */
#include <assert.h> /* panic() */
#include <panic_thread.h> /* panic_thread() */

void
segment_not_present_handler( int *ebp )
{
	int error_code	= *(ebp + 1);
	int eip			= *(ebp + 2);
	int cs			= *(ebp + 3);

	if (cs == SEGSEL_KERNEL_CS) {
		panic("[Kernel mode] Segment not present fault encountered at 0x%x "
				"for segment with index %d", eip, error_code);
	}
	/* TODO: acknowledge signal and call user handler  */

	panic_thread("Unhandled segment not present fault encountered at 0x%x "
			"for segment with index %d", eip, error_code);
}
