/** @file debug_handler.c
 *  @brief Functions for handling debug traps/faults
 */
#include <seg.h>	/* SEGSEL_KERNEL_CS */
#include <assert.h> /* panic() */
#include <panic_thread.h> /* panic_thread() */

void
debug_handler( int *ebp )
{
	int eip	= *(ebp + 1);
	int cs	= *(ebp + 2);

	if (cs == SEGSEL_KERNEL_CS) {
		panic("[Kernel mode] Debug condition encountered at 0x%x."
				"Please contact kernel developers.", eip);
	}
	/* TODO: acknowledge signal and call user handler  */

	panic_thread("Unhandled debug trap or fault encountered at 0x%x", eip);
}
