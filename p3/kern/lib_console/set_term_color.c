/** @file set_term_color.c
 *  @brief Set terminal color syscall handler
 */
#include <asm.h>				/* outb */
#include <console.h>			/* set_term_color */
#include <interrupt_defines.h>	/* INT_CTL_PORT, INT_ACK_CURRENT */

/** @brief Handler for set_term_color syscall.
 *
 *	NOTE: appended _handler to distinguish from console.c function */
int
set_term_color_handler( int color )
{
    /* Acknowledge interrupt */
    outb(INT_CTL_PORT, INT_ACK_CURRENT);

	return set_term_color(color);
}
