/** @file print.S
 *  @brief Assembly wrapper for the print() system call
 *  @author Andre Nascimento (anascime)
 *  @bugs No known bugs
 */

#include <syscall_int.h>

.globl print

print:
	/* Save all callee save registers */
	push %ebp
	mov  %esp, %ebp
	push %edi
	push %ebx
	push %esi

	mov %ebp, %esi
	add $8,   %esi    /* Point %esi to caller arg address */
	int $PRINT_INT    /* Call handler in IDT for print() */

	/* Restore all callee save registers */
	pop %esi
	pop %ebx
	pop %edi
	pop %ebp
	ret



