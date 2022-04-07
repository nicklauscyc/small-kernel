/** @file get_cursor_pos.S
 *  @brief Assembly wrapper for the get_cursor_pos() system call
 *  @author Andre Nascimento (anascime)
 *  @bugs No known bugs
 */

#include <syscall_int.h>

.globl get_cursor_pos

get_cursor_pos:
	/* Save all callee save registers */
	pushl %ebp
	movl  %esp, %ebp
	pushl %edi
	pushl %ebx
	pushl %esi

	leal 8(%ebp), %esi          /* Point %esi to caller arg address */
	int  $GET_CURSOR_POS_INT    /* Call handler in IDT for get_cursor_pos() */

	/* Restore all callee save registers */
	popl %esi
	popl %ebx
	popl %edi
	popl %ebp
	ret



