
#include <asm_interrupt_handler_template.h>

/* should be call w void val? */
CALL_W_RETVAL_HANDLER(vanish)
CALL_W_SINGLE_ARG(task_vanish)
CALL_W_SINGLE_ARG(set_status)
CALL_W_SINGLE_ARG(wait)

CALL_W_RETVAL_HANDLER(fork)
CALL_W_DOUBLE_ARG(exec)

.globl setup_thread_fork
setup_thread_fork:

	pushl %esi
	pushl %edi
	pushl %edx
	pushl %ecx
	pushl %ebx
	call thread_fork
	addl $20, %esp

	ret



CALL_W_RETVAL_HANDLER(setup_thread_fork)

