/** @file get_ticks.S
 *  @brief Assembly wrapper for the get_ticks() system call
 *  @author Andre Nascimento (anascime)
 *  @bugs No known bugs
 */

#include <syscall_int.h>

.globl get_ticks

get_ticks:
	/* Save all callee save registers */
	pushl %ebp
	movl  %esp, %ebp
	pushl %edi
	pushl %ebx
	pushl %esi

	int  $GET_TICKS_INT /* Call handler in IDT for get_ticks() */

	/* Restore all callee save registers */
	popl %esi
	popl %ebx
	popl %edi
	popl %ebp
	ret



