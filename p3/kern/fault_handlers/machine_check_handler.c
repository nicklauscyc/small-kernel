/** @file machine_check_handler.c
 *  @brief Functions for handling overflow traps
 */
#include <seg.h>	/* SEGSEL_KERNEL_CS */
#include <assert.h> /* panic() */
#include <simics.h>

/** @brief Interrupt handler for machine check */
void
machine_check_handler( int *ebp )
{
	int eip			= *(ebp + 1);
	int cs			= *(ebp + 2);

	if (cs == SEGSEL_KERNEL_CS) {
		panic("[Kernel mode] Machine check error encountered at 0x%x.", eip);
	}

	panic("[User mode] Machine check error encountered at 0x%x", eip);
}
