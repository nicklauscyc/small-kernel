/** @file deschedule.S
 *  @brief Assembly wrapper for deschedule() system call
 *  @author Andre Nascimento (anascime)
 *  @bugs No known bugs
 */

#include <syscall_int.h>

.globl deschedule

deschedule:
	/* Save all callee save registers */
	pushl %ebp
	movl  %esp, %ebp
	pushl %edi
	pushl %ebx
	pushl %esi

	movl 8(%ebp), %esi  /* Get first arg and place in %esi */
	int $DESCHEDULE_INT /* Call handler in IDT for deschedule() */

	/* Restore all callee save registers */
	popl %esi
	popl %ebx
	popl %edi
	popl %ebp
	ret



