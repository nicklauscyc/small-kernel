/** @file stack_fault_handler.c
 *  @brief Functions for handling stack faults
 */
#include <seg.h>	/* SEGSEL_KERNEL_CS */
#include <asm.h>			/* outb() */
#include <ureg.h>			/* SWEXN_CAUSE_ */
#include <swexn.h>			/* handle_exn */
#include <assert.h> /* panic() */
#include <panic_thread.h> /* panic_thread() */
#include <interrupt_defines.h> /* INT_CTL_PORT, INT_ACK_CURRENT */

/** @brief Stack fault handler
 *
 *  @param ebp Base pointer to stack of fault handler
 *  @return Void. */
void
stack_fault_handler( int *ebp )
{
	int error_code	= *(ebp + 1);
	int eip			= *(ebp + 2);
	int cs			= *(ebp + 3);

	if (cs == SEGSEL_KERNEL_CS) {
		panic("[Kernel mode] Stack fault encountered at 0x%x "
				"for stack with segment %d", eip, error_code);
	}
	/* If not a kernel exception, acknowledge interrupt */
	outb(INT_CTL_PORT, INT_ACK_CURRENT);

	handle_exn(ebp, SWEXN_CAUSE_STACKFAULT, 0);
	panic_thread("Unhandled segment not present fault encountered at 0x%x "
			"for stack with segment %d", eip, error_code);
}
