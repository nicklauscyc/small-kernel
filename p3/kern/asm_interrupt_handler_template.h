
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

/*
//	mov $SEGSEL_KERNEL_CS, %ax;\
//	mov %ax, %cs;\
//	mov $SEGSEL_KERNEL_DS, %ax;\
//	mov %ax, %ss;\
//	*/
#define CALL_W_RETVAL_HANDLER(HANDLER_NAME)\
\
.globl call_##HANDLER_NAME; /* create the asm function call_HANDLER_NAME */\
\
call_ ## HANDLER_NAME ## :;\
	push %esi; /* Pushes all registers onto the stack */\
	push %edi;\
	push %ebx;\
	push %ebp;\
	call HANDLER_NAME; /* calls timer interrupt handler */\
	pop %ebp;\
	pop %ebx;\
	pop %edi;\
	pop %esi;\
	iret; /* Return to procedure before interrupt */

