/** @file set_status.S
 *  @brief Assembly wrapper for set_status() system call
 *  @author Nicklaus Choo (nchoo)
 *  @bugs No known bugs
 */

#include "../../spec/syscall_int.h"

.globl set_status

set_status:
	/* Save all callee save registers */
	pushl %ebp
	movl  %esp, %ebp
	pushl %edi
	pushl %ebx
	pushl %esi

	movl 8(%ebp), %esi /* Get first arg and place in %esi */
	int $SET_STATUS_INT /* Call handler in IDT for set_status() */

	/* Restore all callee save registers */
	popl %esi
	popl %ebx
	popl %edi
	popl %ebp
	ret



