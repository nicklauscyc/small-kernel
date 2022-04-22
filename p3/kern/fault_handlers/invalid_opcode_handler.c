/** @file invalid_opcode_handler.c
 *  @brief Functions for handling invalid opcode faults
 */
#include <seg.h>	/* SEGSEL_KERNEL_CS */
#include <assert.h> /* panic() */
#include <panic_thread.h> /* panic_thread() */

void
invalid_opcode_handler( int *ebp )
{
	int eip			= *(ebp + 1);
	int cs			= *(ebp + 2);

	if (cs == SEGSEL_KERNEL_CS) {
		panic("[Kernel mode] Invalid opcode fault encountered at 0x%x."
				"Please contact kernel developers.", eip);
	}
	/* TODO: acknowledge signal and call user handler  */

	panic_thread("Unhandled invalid opcode fault encountered at 0x%x", eip);

}
