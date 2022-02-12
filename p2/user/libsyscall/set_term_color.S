/** @file set_term_color.S
 *  @brief Assembly wrapper for set_term_color() system call
 *  @author Andre Nascimento (anascime)
 *  @bugs No known bugs
 */

#include <syscall_int.h>

.globl set_term_color

wait:
	/* Save all callee save registers */
	pushl %ebp
	movl  %esp, %ebp
	pushl %edi
	pushl %ebx
	pushl %esi

	movl 8(%ebp), %esi      /* Get first arg and place in %esi */
	int $SET_TERM_COLOR_INT /* Call handler in IDT for set_term_color() */

	/* Restore all callee save registers */
	popl %esi
	popl %ebx
	popl %edi
	popl %ebp
	ret



