/** @file asm_interrupt_template_handler.h
 *  @brief Collection of MACROS providing templates for asm wrappers to
 *		   syscalls and fault handlers.*/

#include <seg.h> /* SEGSEL_KERNEL_{CS, DS} */


/** @define CALL_HANDLER(HANDLER_NAME)
 *  @brief Assembly wrapper to call interupt handlers that do not service
 *         syscalls (hence the need to save _all_ general registers)
 *
 *  @param HANDLER_NAME name of handler function to call
 */
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
	mov %ss, %ax;\
	mov %ax, %ds;\
	mov %ax, %es;\
	mov %ax, %fs;\
	mov %ax, %gs;\
	call HANDLER_NAME; /* calls timer interrupt handler */\
	popl %gs;\
	popl %fs;\
	popl %es;\
	popl %ds;\
	popa; /* Restores all registers onto the stack */\
	iret; /* Return to procedure before interrupt */

/** @define CALL_FAULT_HANDLER_TEMPLATE(HANDLER_NAME)
 *  @brief Assembly wrapper to call interupt handlers that do not service
 *         syscalls (hence the need to save _all_ general registers)
 *
 *  @param HANDLER_NAME name of handler function to call
 */
#define CALL_FAULT_HANDLER_TEMPLATE(HANDLER_SPECIFIC_CODE)\
\
	/* Make use of ebp for argument access */\
	pushl %ebp;\
	movl %esp, %ebp;\
\
	/* Save all registers */\
	pusha;\
\
	/* Save all segment registers on the stack */\
	pushl %ds;\
	pushl %es;\
	pushl %fs;\
	pushl %gs;\
\
	/* set the new values for ds, es, fs, gs */\
	/* TODO fix so clang does not complain */\
	mov %ss, %ax;\
	mov %ax, %ds;\
	mov %ax, %es;\
	mov %ax, %fs;\
	mov %ax, %gs;\
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
	popa;\
\
	/* Restore ebp */\
	popl %ebp;\
\
	/* Return to procedure before interrupt */\
	iret;

/** @define CALL_FAULT_HANDLER_TEMPLATE(HANDLER_NAME)
 *  @brief Assembly wrapper to call fault handlers which place
 *		   an error code on the stack.
 *
 *  @param HANDLER_NAME name of handler function to call
 */
#define CALL_FAULT_HANDLER_TEMPLATE_W_ERROR(HANDLER_SPECIFIC_CODE)\
\
	/* Make use of ebp for argument access */\
	pushl %ebp;\
	movl %esp, %ebp;\
\
	/* Save all registers */\
	pusha;\
\
	/* Save all segment registers on the stack */\
	pushl %ds;\
	pushl %es;\
	pushl %fs;\
	pushl %gs;\
\
	/* set the new values for ds, es, fs, gs */\
	/* TODO fix so clang does not complain */\
	mov %ss, %ax;\
	mov %ax, %ds;\
	mov %ax, %es;\
	mov %ax, %fs;\
	mov %ax, %gs;\
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
	popa;\
\
	/* Restore ebp */\
	popl %ebp;\
\
	/* Return to procedure before interrupt */\
	addl $4, %esp;\
	iret;

/** @define CALL_HANDLER_TEMPLATE(HANDLER_SPECIFIC_CODE)
 *  @brief Wrapper code for syscall assembly wrappers that saves and restores
 *         general and segment registers.
 *
 *  @param HANDLER_SPECIFIC_CODE syscall handler specific wrapper code
 */
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
	mov %ss, %ax;\
	mov %ax, %ds;\
	mov %ax, %es;\
	mov %ax, %fs;\
	mov %ax, %gs;\
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
 *
 *  @param HANDLER_NAME handler to call
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

/** @define CALL_FAULT_HANDLER(HANDLER_NAME)
 *  @brief Assembly wrapper for a fault handler.
 *
 *  Ebp is passed as an argument to c handler which can then inspect the
 *  stack for values it desires.
 *
 *  @param HANDLER_NAME handler name to call
 */
#define CALL_FAULT_HANDLER(HANDLER_NAME)\
\
/* Declare and define asm function call_HANDLER_NAME */\
.globl call_##HANDLER_NAME;\
call_ ## HANDLER_NAME ## :;\
\
	CALL_FAULT_HANDLER_TEMPLATE(SINGLE_MACRO_ARG_W_COMMAS\
	(\
		pushl %ebp;		    /* push ebp onto stack */\
		call HANDLER_NAME;  /* calls syscall handler */\
		addl $4, %esp;		/* ignore arguments */\
	))

/** @define CALL_FAULT_HANDLER_W_ERROR(HANDLER_NAME)
 *  @brief Assembly wrapper for a fault handler with error code.
 *
 *  Ebp is passed as an argument to c handler which can then inspect the
 *  stack for values it desires.
 *
 *  @param HANDLER_NAME handler name to call
 */
#define CALL_FAULT_HANDLER_W_ERROR(HANDLER_NAME)\
\
/* Declare and define asm function call_HANDLER_NAME */\
.globl call_##HANDLER_NAME;\
call_ ## HANDLER_NAME ## :;\
\
	CALL_FAULT_HANDLER_TEMPLATE_W_ERROR(SINGLE_MACRO_ARG_W_COMMAS\
	(\
		pushl %ebp;		    /* push ebp onto stack */\
		call HANDLER_NAME;  /* calls syscall handler */\
		addl $4, %esp;		/* ignore arguments */\
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
		addl $8, %esp;      /* ignore arguments */\
	))


/** @def CALL_W_FOUR_ARG(HANDLER_NAME)
 *  @brief Macro for assembly wrapper for calling a syscall with 4 arguments
 *
 *  @param HANDLER_NAME handler name to call
 */
#define CALL_W_FOUR_ARG(HANDLER_NAME)\
\
/* Declare and define asm function call_HANDLER_NAME */\
.globl call_##HANDLER_NAME;\
call_ ## HANDLER_NAME ## :;\
\
	CALL_HANDLER_TEMPLATE(SINGLE_MACRO_ARG_W_COMMAS\
	(\
		pushl 12(%esi);     /* push 4th argument onto stack */\
		pushl 8(%esi);      /* push 3rd argument onto stack */\
		pushl 4(%esi);      /* push 2nd argument onto stack */\
		pushl (%esi);       /* push 1st argument onto stack */\
		call HANDLER_NAME;  /* calls syscall handler */\
		addl $16, %esp;     /* ignore arguments */\
	))


