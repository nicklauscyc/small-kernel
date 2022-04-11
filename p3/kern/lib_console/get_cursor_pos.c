/** @file get_cursor_pos.c
 *  @brief Get cursor position syscall handler
 */
#include <asm.h>				/* outb */
#include <console.h>			/* get_cursor */
#include <memory_manager.h>		/* is_user_pointer_valid */
#include <interrupt_defines.h>	/* INT_CTL_PORT, INT_ACK_CURRENT */

/** @brief Handler for get_cursor_pos syscall. */
int
get_cursor_pos( int *row, int *col )
{
    /* Acknowledge interrupt */
    outb(INT_CTL_PORT, INT_ACK_CURRENT);

	if (!is_valid_user_pointer(row, READ_WRITE)
			|| !is_valid_user_pointer(col, READ_WRITE))
		return -1;

	get_cursor(row, col);
	return 0;
}
