/** @file make_runnable.S
 *  @brief Assembly wrapper for make_runnable() system call
 *  @author Andre Nascimento (anascime)
 *  @bugs No known bugs
 */

#include <syscall_int.h>

.globl make_runnable

make_runnable:
	/* Save all callee save registers */
	pushl %ebp
	movl  %esp, %ebp
	pushl %edi
	pushl %ebx
	pushl %esi

	movl 8(%ebp), %esi      /* Get first arg and place in %esi */
	int $MAKE_RUNNABLE_INT  /* Call handler in IDT for make_runnable() */

	/* Restore all callee save registers */
	popl %esi
	popl %ebx
	popl %edi
	popl %ebp
	ret



