/** @file vanish.S
 *  @brief Assembly wrapper for vanish() system call
 *  @author Nicklaus Choo (nchoo)
 *  @bugs No known bugs
 */

#include <syscall_int.h>

.globl vanish

vanish:
	/* Save all callee save registers */
	pushl %ebp
	movl  %esp, %ebp
	pushl %edi
	pushl %ebx
	pushl %esi

	int $VANISH_INT /* Call handler in IDT for vanish() */

	/* Restore all callee save registers */
	popl %esi
	popl %ebx
	popl %edi
	movl %ebp, %esp /* Andre: Think we don't need this */
	popl %ebp
	ret



