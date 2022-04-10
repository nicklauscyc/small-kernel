/** @file stack_fault_handler.c
 *  @brief Functions for handling stack faults
 */
#include <seg.h>	/* SEGSEL_KERNEL_CS */
#include <assert.h> /* panic() */

void
stack_fault_handler( int error_code, int eip, int cs )
{
	if (cs == SEGSEL_KERNEL_CS) {
		panic("[Kernel mode] Stack fault encountered at 0x%x "
				"for stack with segment %d", eip, error_code);
	}
	/* TODO: acknowledge signal and call user handler  */

	panic("Unhandled segment not present fault encountered at 0x%x "
			"for stack with segment %d", eip, error_code);
}
