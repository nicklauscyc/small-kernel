/** @file remove_pages.S
 *  @brief Assembly wrapper for remove_pages() system call
 *  @author Nicklaus Choo (nchoo)
 *  @bugs No known bugs
 */

#include <syscall_int.h>

.globl remove_pages

remove_pages:
	/* Save all callee save registers */
	pushl %ebp
	movl  %esp, %ebp
	pushl %edi
	pushl %ebx
	pushl %esi

	movl 8(%ebp), %esi /* Get first arg and place in %esi */
	int $REMOVE_PAGES_INT /* Call handler in IDT for remove_pages() */

	/* Restore all callee save registers */
	popl %esi
	popl %ebx
	popl %edi
	popl %ebp
	ret



