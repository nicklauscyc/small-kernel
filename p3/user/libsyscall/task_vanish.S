/** @file task_vanish.S
 *  @brief Assembly wrapper for task_vanish() system call
 *  @author Nicklaus Choo (nchoo)
 *  @bugs No known bugs
 */

#include <syscall_int.h>

.globl task_vanish

task_vanish:
	/* Save all callee save registers */
	pushl %ebp
	movl  %esp, %ebp
	pushl %edi
	pushl %ebx
	pushl %esi

	movl 8(%ebp), %esi      /* Get first arg and place in %esi */
	int $TASK_VANISH_INT    /* Call handler in IDT for task_vanish() */

	/* Restore all callee save registers */
	popl %esi
	popl %ebx
	popl %edi
	popl %ebp
	ret



