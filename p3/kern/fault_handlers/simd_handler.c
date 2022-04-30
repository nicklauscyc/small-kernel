/** @file simd_handler.c
 *  @brief Functions for handling simd faults
 */
#include <seg.h>	/* SEGSEL_KERNEL_CS */
#include <asm.h>			/* outb() */
#include <ureg.h>			/* SWEXN_CAUSE_ */
#include <swexn.h>			/* handle_exn */
#include <assert.h> /* panic() */
#include <panic_thread.h> /* panic_thread() */
#include <interrupt_defines.h> /* INT_CTL_PORT, INT_ACK_CURRENT */

/** @brief SIMD handler
 *
 *  @param ebp Base pointer to stack of fault handler
 *  @return Void. */
void
simd_handler( int *ebp )
{
	int eip	= *(ebp + 1);
	int cs	= *(ebp + 2);

	if (cs == SEGSEL_KERNEL_CS) {
		panic("[Kernel mode] SIMD operation encountered at 0x%x." , eip);
	}

	/* If not a kernel exception, acknowledge interrupt */
	outb(INT_CTL_PORT, INT_ACK_CURRENT);

	handle_exn(ebp, SWEXN_CAUSE_SIMDFAULT, 0);
	panic_thread("Unhandled simd fault at instruction 0x%x", eip);
}
