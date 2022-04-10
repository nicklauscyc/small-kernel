/** @file stack_fault_handler.c
 *  @brief Functions for stack faults
 */
#include <seg.h>	/* SEGSEL_KERNEL_CS */
#include <assert.h> /* panic() */
#include <simics.h>

void
stack_fault_handler( int error_code, int cs, int eip )
{
	if (cs == SEGSEL_KERNEL_CS) {
		lprintf("[Kernel mode] Stack segment fault encountered error at "
		        "0x%x.", eip);
	}
	/* TODO: acknowledge signal and call user handler  */

	lprintf("[User mode] Stack segment fault encountered at 0x%x", eip);
}
