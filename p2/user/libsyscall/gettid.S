/** @file gettid.S
 *  @brief Assembly wrapper for the gettid() system call
 *  @author Andre Nascimento (anascime)
 *  @bugs No known bugs
 */

#include <syscall_int.h>

.globl gettid

gettid:
	/* Save all callee save registers */
	pushl %ebp
	movl  %esp, %ebp
	pushl %edi
	pushl %ebx
	pushl %esi

	int  $GETTID_INT		/* Call handler in IDT for gettid() */

	/* Restore all callee save registers */
	popl %esi
	popl %ebx
	popl %edi
	popl %ebp
	ret



