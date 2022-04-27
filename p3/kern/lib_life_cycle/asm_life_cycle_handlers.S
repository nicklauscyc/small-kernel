
#include <asm_interrupt_handler_template.h>

/* should be call w void val? */
CALL_W_RETVAL_HANDLER(vanish)
CALL_W_SINGLE_ARG(task_vanish)
CALL_W_SINGLE_ARG(set_status)
CALL_W_SINGLE_ARG(wait)

CALL_W_RETVAL_HANDLER(fork)
CALL_W_DOUBLE_ARG(exec)

.globl call_thread_fork

call_thread_fork:
	pusha /* Pushes all registers onto the stack */
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs
	/* set the new values for ds, es, fs, gs */
	mov %ss, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs
	call thread_fork /* calls thread fork handler */
	popl %gs
	popl %fs
	popl %es
	popl %ds
	movl %eax, 28(%esp) /* Hijack popa into preserving eax */
	popa /* Restores all registers onto the stack */
	iret /* Return to procedure before interrupt */

