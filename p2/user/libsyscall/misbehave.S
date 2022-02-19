/** @file misbehave.S
 *  @brief Assembly wrapper for misbehave() system call
 *  @author Nicklaus Choo (nchoo)
 *  @bugs No known bugs
 */

#include <syscall_int.h>

.globl misbehave

misbehave:
	/* Save all callee save registers */
	pushl %ebp
	movl  %esp, %ebp
	pushl %edi
	pushl %ebx
	pushl %esi

	movl 8(%ebp), %esi /* Get first arg and place in %esi */
	int $MISBEHAVE_INT /* Call handler in IDT for misbehave() */

	/* Restore all callee save registers */
	popl %esi
	popl %ebx
	popl %edi
	popl %ebp
	ret



