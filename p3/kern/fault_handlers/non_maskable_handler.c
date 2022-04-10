/** @file non_maskable_handler.c
 *  @brief Functions for NMIs
 */
#include <seg.h>	/* SEGSEL_KERNEL_CS */
#include <assert.h> /* panic() */
#include <simics.h>

void
non_maskable_handler( int cs, int eip )
{
	if (cs == SEGSEL_KERNEL_CS) {
		lprintf("[Kernel mode] NMI encountered at 0x%x.", eip);
	}
	/* TODO: acknowledge signal and call user handler  */

	lprintf("[User mode] NMI encountered at 0x%x", eip);
}
