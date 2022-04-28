/** @file set_cursor_pos.c
 *  @brief Set cursor position syscall handler
 */
#include <asm.h>				/* outb */
#include <console.h>			/* set_cursor */
#include <interrupt_defines.h>	/* INT_CTL_PORT, INT_ACK_CURRENT */

/** @brief Handler for set_cursor_pos syscall
 *
 *  @param row New row for cursor
 *  @param col New col for cursor
 *  @return 0 on success, negative value on error */
int
set_cursor_pos( int row, int col )
{
    /* Acknowledge interrupt */
    outb(INT_CTL_PORT, INT_ACK_CURRENT);

	return set_cursor(row, col);
}
