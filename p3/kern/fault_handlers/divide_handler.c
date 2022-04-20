/** @file divide_handler.c
 *  @brief Functions for handling division faults
 */
#include <seg.h>	/* SEGSEL_KERNEL_CS */
#include <assert.h> /* panic() */
#include <panic_thread.h> /* panic_thread() */

/** @brief Prints out the offending address on and calls panic()
 *
 *  @return Void.
 */
void
divide_handler( int eip, int cs )
{
	if (cs == SEGSEL_KERNEL_CS) {
		panic("[Kernel mode] Divide by 0 exception at 0x%x."
				"Please contact kernel developers.", eip);
	}
	/* TODO: acknowledge signal and call user handler  */

	panic_thread("Unhandled divide by 0 exception at instruction 0x%x", eip);
}
