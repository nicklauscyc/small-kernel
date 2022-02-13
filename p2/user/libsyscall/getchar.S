/** @file getchar.S
 *  @brief Assembly wrapper for the getchar() system call
 *  @author Andre Nascimento (anascime)
 *  @bugs No known bugs
 */

#include <syscall_int.h>

.globl getchar

getchar:
	/* Save all callee save registers */
	pushl %ebp
	movl  %esp, %ebp
	pushl %edi
	pushl %ebx
	pushl %esi

	int  $GETCHAR_INT   /* Call handler in IDT for getchar() */

	/* Restore all callee save registers */
	popl %esi
	popl %ebx
	popl %edi
	popl %ebp
	ret



