/** @file sleep.S
 *  @brief Assembly wrapper for sleep() system call
 *  @author Andre Nascimento (anascime)
 *  @bugs No known bugs
 */

#include <syscall_int.h>

.globl sleep

sleep:
	/* Save all callee save registers */
	pushl %ebp
	movl  %esp, %ebp
	pushl %edi
	pushl %ebx
	pushl %esi

	movl 8(%ebp), %esi  /* Get first arg and place in %esi */
	int $SLEEP_INT      /* Call handler in IDT for sleep() */

	/* Restore all callee save registers */
	popl %esi
	popl %ebx
	popl %edi
	popl %ebp
	ret



