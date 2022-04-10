/** @file divide_handler.c
 *  @brief Functions for handling division faults
 */
#include <assert.h> /* panic() */
#include <install_handler.h> /* install_handler_in_idt() */

/** @brief Prints out the offending address on and calls panic()
 *
 *  @return Void.
 */
void
divide_handler( int cs, int eip )
{
	if (cs == KERNEL_CODE_SEGSEL) {
		panic("[Kernel mode] Divide by 0 exception at 0x%x."
				"Please contact kernel developers.", eip);
	}

	/* TODO: acknowledge signal and call user handler  */


	panic("Divide by 0 exception at instruction 0x%x", eip);
}
