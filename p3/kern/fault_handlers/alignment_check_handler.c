/** @file alignment_check_handler.c
 *  @brief Functions for alignment check faults
 */
#include <seg.h>			/* SEGSEL_KERNEL_CS */
#include <asm.h>			/* outb() */
#include <ureg.h>			/* SWEXN_CAUSE_ */
#include <swexn.h>			/* handle_exn */
#include <assert.h>			/* panic() */
#include <panic_thread.h>	/* panic_thread() */
#include <interrupt_defines.h> /* INT_CTL_PORT, INT_ACK_CURRENT */

/** @brief Alignment check handler
 *
 *  @param ebp Base pointer to stack of fault handler
 *  @return Void.*/
void
alignment_check_handler( int *ebp )
{
	int error_code	= *(ebp + 1);
	int eip			= *(ebp + 2);
	int cs			= *(ebp + 3);

	affirm(error_code == 0);
	if (cs == SEGSEL_KERNEL_CS) {
		panic("[Kernel mode] Alignment check fault  encountered error at "
		        "0x%x.", eip);
	}

	/* If not a kernel exception, acknowledge interrupt */
	outb(INT_CTL_PORT, INT_ACK_CURRENT);

	handle_exn(ebp, SWEXN_CAUSE_ALIGNFAULT, 0);
	panic_thread("Unhandled alignment check fault encountered at 0x%x", eip);
}
