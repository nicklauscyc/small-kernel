/** @file run_thread.S
 *  @brief Assembly wrapper for the run_thread() system call
 *  @author Nicklaus Choo (nchoo)
 *  @bugs No known bugs
 */

#include <syscall_int.h>

.globl run_thread

run_thread:
	/* Save all callee save registers */
	pushl %ebp
	movl  %esp, %ebp
	pushl %edi
	pushl %ebx
	pushl %esi

	movl 8(%ebp), %esp /* Set the new stack pointer */
	movl 12(%ebp), %esi /* Move arg into %esi */
	pushl %esi /* put arg for func() onto stack */
	call func /* call func() */

	/* Restore all callee save registers */
	popl %esi
	popl %ebx
	popl %edi
	popl %ebp
	ret



