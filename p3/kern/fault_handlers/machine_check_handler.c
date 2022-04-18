/** @file machine_check_handler.c
 *  @brief Functions for handling overflow traps
 */
#include <seg.h>	/* SEGSEL_KERNEL_CS */
#include <assert.h> /* panic() */
#include <simics.h>

void
machine_check_handler( int eip, int cs )
{
	if (cs == SEGSEL_KERNEL_CS) {
		panic("[Kernel mode] Machine check error encountered at 0x%x.", eip);
	}
	/* TODO: acknowledge signal and call user handler  */

	panic("[User mode] Machine check error encountered at 0x%x", eip);
}
