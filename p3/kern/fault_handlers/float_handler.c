/** @file float_handler.c
 *  @brief Functions for handling device not available faults
 */
#include <seg.h>	/* SEGSEL_KERNEL_CS */
#include <asm.h>			/* outb() */
#include <ureg.h>			/* SWEXN_CAUSE_ */
#include <swexn.h>			/* handle_exn */
#include <assert.h> /* panic() */
#include <panic_thread.h> /* panic_thread() */
#include <interrupt_defines.h> /* INT_CTL_PORT, INT_ACK_CURRENT */

void
float_handler( int *ebp )
{
	int eip	= *(ebp + 1);
	int cs	= *(ebp + 2);

	if (cs == SEGSEL_KERNEL_CS) {
		panic("[Kernel mode] Floating point operation encountered at 0x%x."
				"Please contact kernel developers.", eip);
	}

	/* If not a kernel exception, acknowledge interrupt */
	outb(INT_CTL_PORT, INT_ACK_CURRENT);

	handle_exn(ebp, SWEXN_CAUSE_NOFPU, 0);
	panic_thread("Unhandled device not available fault "
			"(due to floating-point op) at instruction 0x%x", eip);
}
