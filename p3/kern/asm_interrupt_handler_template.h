#include <seg.h>

/** @brief Macro to call a particular function handler with name
 * 		   HANDLER_NAME
 *
 *	Macro assembly instructions require a ; after each line
 */

#include <x86/seg.h> /* SEGSEL_KERNEL_{CS, DS} */

#define CALL_HANDLER(HANDLER_NAME)\
\
.globl call_##HANDLER_NAME; /* create the asm function call_HANDLER_NAME */\
\
call_ ## HANDLER_NAME ## :;\
	pusha; /* Pushes all registers onto the stack */\
	call HANDLER_NAME; /* calls timer interrupt handler */\
	popa; /* Restores all registers onto the stack */\
	iret; /* Return to procedure before interrupt */

#define CALL_W_RETVAL_HANDLER(HANDLER_NAME)\
\
.globl call_##HANDLER_NAME; /* create the asm function call_HANDLER_NAME */\
\
call_ ## HANDLER_NAME ## :;\
	/* Save all callee save registers */\
	pushl %ebp;\
	movl  %esp, %ebp;\
	pushl %edi;\
	pushl %ebx;\
	pushl %esi;\
	call HANDLER_NAME; /* calls syscall handler */\
	/* Restore all callee save registers */\
	popl %esi;\
	popl %ebx;\
	popl %edi;\
	popl %ebp;\
   	iret; /* Return to procedure before interrupt */

