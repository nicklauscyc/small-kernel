/** @file simd_handler.c
 *  @brief Functions for handling simd faults
 */
#include <seg.h>	/* SEGSEL_KERNEL_CS */
#include <assert.h> /* panic() */
#include <panic_thread.h> /* panic_thread() */

void
simd_handler( int *ebp )
{
	int eip	= *(ebp + 1);
	int cs	= *(ebp + 2);

	if (cs == SEGSEL_KERNEL_CS) {
		panic("[Kernel mode] SIMD operation encountered at 0x%x." , eip);
	}

	panic_thread("Unhandled simd fault at instruction 0x%x", eip);
}
