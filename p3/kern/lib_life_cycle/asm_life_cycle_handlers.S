
#include <asm_interrupt_handler_template.h>

CALL_W_RETVAL_HANDLER(vanish)
CALL_W_SINGLE_ARG(task_vanish)
CALL_W_SINGLE_ARG(set_status)
CALL_W_SINGLE_ARG(wait)
CALL_W_RETVAL_HANDLER(fork)
CALL_W_DOUBLE_ARG(exec)


/** @brief Sets up the stack for thread_fork.
 */
.globl call_thread_fork

call_thread_fork:
	pusha /* Pushes all registers onto the stack */
	push %ds
	push %es
	push %fs
	push %gs
	/* set the new values for ds, es, fs, gs */
	mov %ss, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs
	call thread_fork /* calls thread fork handler */
	pop %gs
	pop %fs
	pop %es
	pop %ds
	movl %eax, 28(%esp) /* Hijack popa into preserving eax */
	popa /* Restores all registers onto the stack */
	iret /* Return to procedure before interrupt */

