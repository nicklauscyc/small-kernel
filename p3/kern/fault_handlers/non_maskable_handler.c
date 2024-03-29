/** @file non_maskable_handler.c
 *  @brief Functions for NMIs
 */
#include <seg.h>	/* SEGSEL_KERNEL_CS */
#include <assert.h> /* panic() */
#include <simics.h>

/** @brief Non maskable interrupt handler
 *
 *  @param ebp Base pointer to stack of fault handler
 *  @return Void. */
void
non_maskable_handler( int *ebp )
{
	int eip	= *(ebp + 1);
	int cs	= *(ebp + 2);

	if (cs == SEGSEL_KERNEL_CS) {
		panic("[Kernel mode] NMI encountered at 0x%x.", eip);
	}

	panic("[User mode] NMI encountered at 0x%x", eip);
}
