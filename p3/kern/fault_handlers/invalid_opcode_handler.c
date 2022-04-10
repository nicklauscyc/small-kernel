/** @file invalid_opcode_handler.c
 *  @brief Functions for handling invalid opcode faults
 */
#include <seg.h>	/* SEGSEL_KERNEL_CS */
#include <assert.h> /* panic() */

void
invalid_opcode_handler( int cs, int eip )
{
	if (cs == SEGSEL_KERNEL_CS) {
		panic("[Kernel mode] Invalid opcode fault encountered at 0x%x."
				"Please contact kernel developers.", eip);
	}
	/* TODO: acknowledge signal and call user handler  */

	panic("Unhandled invalid opcode fault encountered at 0x%x", eip);

}
