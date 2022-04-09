/** @file set_cursor_pos.c
 *  @brief Set cursor position syscall handler
 */
#include <asm.h>				/* outb */
#include <console.h>			/* set_cursor */
#include <interrupt_defines.h>	/* INT_CTL_PORT, INT_ACK_CURRENT */

/** @brief Handler for set_cursor_pos syscall. */
int
set_cursor_pos( int row, int col )
{
    /* Acknowledge interrupt */
    outb(INT_CTL_PORT, INT_ACK_CURRENT);

	return set_cursor(row, col);
}
