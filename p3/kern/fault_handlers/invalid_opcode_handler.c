/** @file invalid_opcode_handler.c
 *  @brief Functions for handling invalid opcode faults
 */
#include <seg.h>			/* SEGSEL_KERNEL_CS */
#include <asm.h>			/* outb() */
#include <ureg.h>			/* SWEXN_CAUSE_ */
#include <swexn.h>			/* handle_exn */
#include <assert.h> 		/* panic() */
#include <panic_thread.h>	/* panic_thread() */
#include <interrupt_defines.h> /* INT_CTL_PORT, INT_ACK_CURRENT */

/** @brief Invalid opcode handler
 *
 *  @param ebp Base pointer to stack of fault handler
 *  @return Void. */
void
invalid_opcode_handler( int *ebp )
{
	int eip			= *(ebp + 1);
	int cs			= *(ebp + 2);

	if (cs == SEGSEL_KERNEL_CS) {
		panic("[Kernel mode] Invalid opcode fault encountered at 0x%x."
				"Please contact kernel developers.", eip);
	}
	/* If not a kernel exception, acknowledge interrupt */
	outb(INT_CTL_PORT, INT_ACK_CURRENT);

	handle_exn(ebp, SWEXN_CAUSE_OPCODE, 0);
	panic_thread("Unhandled invalid opcode fault encountered at 0x%x", eip);

}
