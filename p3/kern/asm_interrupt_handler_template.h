#include <seg.h>

/** @brief Macro to call a particular function handler with name
 * 		   HANDLER_NAME
 *
 *	Macro assembly instructions require a ; after each line
 */

#include <x86/seg.h> /* SEGSEL_KERNEL_{CS, DS} */

/* TODO to what extent do we still need this handler ? */
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

#define CALL_HANDLER_TEMPLATE(HANDLER_SPECIFIC_CODE)\
\
	/* Save all callee save registers */\
	pushl %ebp;\
	movl %esp, %ebp;\
	pushl %edi;\
	pushl %ebx;\
	pushl %esi;\
\
	/* Save all segment registers on the stack */\
	pushl %ds;\
	pushl %es;\
	pushl %fs;\
	pushl %gs;\
\
	/* set the new values for ds, es, fs, gs */\
	/* TODO fix so clang does not complain */\
	movl %ss, %ax;\
	movl %ax, %ds;\
	movl %ax, %es;\
	movl %ax, %fs;\
	movl %ax, %gs;\
\
	HANDLER_SPECIFIC_CODE\
\
	/* Restores all segment registers from the stack */\
	popl %gs;\
	popl %fs;\
	popl %es;\
	popl %ds;\
\
	/* Restores all callee save registers from the stack */\
	popl %esi;\
	popl %ebx;\
	popl %edi;\
	popl %ebp;\
\
	/* Return to procedure before interrupt */\
	iret;

/** @define CALL_W_RETVAL_HANDLER(HANDLER_NAME)
 *  @brief Assembly wrapper for a syscall handler that returns a value,
 *         takes in no arguments
 */
#define CALL_W_RETVAL_HANDLER(HANDLER_NAME)\
\
/* Declare and define asm function call_HANDLER_NAME */\
.globl call_##HANDLER_NAME;\
call_ ## HANDLER_NAME ## :;\
\
	CALL_HANDLER_TEMPLATE\
	(\
		/* Call syscall handler */\
		call HANDLER_NAME;\
	)

#define SINGLE_MACRO_ARG_W_COMMAS(...) __VA_ARGS__

/** @define CALL_W_SINGLE_ARG(HANDLER_NAME)
 *  @brief Assembly wrapper for a syscall handler that takes in 1 argument.
 *
 *  Call convention dictates single argument is found in %esi
 *
 *  @param HANDLER_NAME handler name to call
 */
#define CALL_W_SINGLE_ARG(HANDLER_NAME)\
\
/* Declare and define asm function call_HANDLER_NAME */\
.globl call_##HANDLER_NAME;\
call_ ## HANDLER_NAME ## :;\
\
	CALL_HANDLER_TEMPLATE(SINGLE_MACRO_ARG_W_COMMAS\
	(\
		pushl %esi;         /* push argument onto stack */\
		call HANDLER_NAME;  /* calls syscall handler */\
		addl $4, %esp;      /* ignore argument */\
	))

/** @def CALL_W_DOUBLE_ARG(HANDLER_NAME)
 *  @brief Macro for assembly wrapper for calling a syscall with 2 arguments
 *
 *  @param HANDLER_NAME handler name to call
 */
#define CALL_W_DOUBLE_ARG(HANDLER_NAME)\
\
/* Declare and define asm function call_HANDLER_NAME */\
.globl call_##HANDLER_NAME;\
call_ ## HANDLER_NAME ## :;\
\
	CALL_HANDLER_TEMPLATE(SINGLE_MACRO_ARG_W_COMMAS\
	(\
		pushl 4(%esi);      /* push 2nd argument onto stack */\
		pushl (%esi);       /* push 1st argument onto stack */\
		call HANDLER_NAME;  /* calls syscall handler */\
		addl $4, %esp;      /* ignore argument */\
	))



