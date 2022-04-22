/** @file divide_handler.c
 *  @brief Functions for handling division faults
 */
#include <seg.h>	/* SEGSEL_KERNEL_CS */
#include <asm.h>			/* outb() */
#include <ureg.h>			/* SWEXN_CAUSE_ */
#include <swexn.h>			/* handle_exn */
#include <assert.h> /* panic() */
#include <panic_thread.h> /* panic_thread() */
#include <interrupt_defines.h> /* INT_CTL_PORT, INT_ACK_CURRENT */

/** @brief Prints out the offending address on and calls panic()
 *
 *  @return Void.
 */
void
divide_handler( int *ebp )
{
	int eip	= *(ebp + 1);
	int cs	= *(ebp + 2);

	if (cs == SEGSEL_KERNEL_CS) {
		panic("[Kernel mode] Divide by 0 exception at 0x%x."
				"Please contact kernel developers.", eip);
	}
	/* If not a kernel exception, acknowledge interrupt */
	outb(INT_CTL_PORT, INT_ACK_CURRENT);

	handle_exn(ebp, SWEXN_CAUSE_DIVIDE, 0);
	panic_thread("Unhandled divide by 0 exception at instruction 0x%x", eip);
}
