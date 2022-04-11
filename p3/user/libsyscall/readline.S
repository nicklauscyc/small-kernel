/** @file readline.S
 *  @brief Assembly wrapper for the readline() system call
 *  @author Andre Nascimento (anascime)
 *  @bugs No known bugs
 */

#include <syscall_int.h>

.globl readline

readline:
	/* Save all callee save registers */
	pushl %ebp
	movl  %esp, %ebp
	pushl %edi
	pushl %ebx
	pushl %esi

	leal 8(%ebp), %esi  /* Point %esi to caller arg address */
	int  $READLINE_INT	/* Call handler in IDT for readline() */

	/* Restore all callee save registers */
	popl %esi
	popl %ebx
	popl %edi
	popl %ebp
	ret



