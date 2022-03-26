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
	pushl %ds;\
	pushl %es;\
	pushl %fs;\
	pushl %gs;\
	/* set the new values for ds, es, fs, gs */\
	movl %ss, %ax;\
	movl %ax, %ds;\
	movl %ax, %es;\
	movl %ax, %fs;\
	movl %ax, %gs;\
	call HANDLER_NAME; /* calls timer interrupt handler */\
	popl %gs;\
	popl %fs;\
	popl %es;\
	popl %ds;\
	popa; /* Restores all registers onto the stack */\
	iret; /* Return to procedure before interrupt */

#define CALL_W_RETVAL_HANDLER(HANDLER_NAME)\
\
.globl call_##HANDLER_NAME; /* create the asm function call_HANDLER_NAME */\
\
call_ ## HANDLER_NAME ## :;\
	/* Save all callee save registers */\
	pushl %ebp;\
	movl %esp, %ebp;\
	pushl %edi;\
	pushl %ebx;\
	pushl %esi;\
	pushl %ds;\
	pushl %es;\
	pushl %fs;\
	pushl %gs;\
	/* set the new values for ds, es, fs, gs */\
	movl %ss, %ax;\
	movl %ax, %ds;\
	movl %ax, %es;\
	movl %ax, %fs;\
	movl %ax, %gs;\
	call HANDLER_NAME; /* calls syscall handler */\
	/* Restore all callee save registers */\
	popl %gs;\
	popl %fs;\
	popl %es;\
	popl %ds;\
	popl %esi;\
	popl %ebx;\
	popl %edi;\
	popl %ebp;\
   	iret; /* Return to procedure before interrupt */


/* Call convention dictates single argument is found is %esi */
#define CALL_W_SINGLE_ARG(HANDLER_NAME)\
\
.globl call_##HANDLER_NAME; /* create the asm function call_HANDLER_NAME */\
\
call_ ## HANDLER_NAME ## :;\
	/* Save all callee save registers */\
	pushl %ebp;\
	movl %esp, %ebp;\
	pushl %edi;\
	pushl %ebx;\
	pushl %esi;\
	pushl %ds;\
	pushl %es;\
	pushl %fs;\
	pushl %gs;\
	/* set the new values for ds, es, fs, gs */\
	movl %ss, %ax;\
	movl %ax, %ds;\
	movl %ax, %es;\
	movl %ax, %fs;\
	movl %ax, %gs;\
    \
    pushl %esi;         /* push argument onto stack */\
	call HANDLER_NAME;  /* calls syscall handler */\
    addl $4, %esp;      /* ignore argument */\
    \
	/* Restore all callee save registers */\
	popl %gs;\
	popl %fs;\
	popl %es;\
	popl %ds;\
	popl %esi;\
	popl %ebx;\
	popl %edi;\
	popl %ebp;\
   	iret; /* Return to procedure before interrupt */
