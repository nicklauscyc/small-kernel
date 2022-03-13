
/** @brief Macro to call a particular function handler with name
 * 		   HANDLER_NAME
 *
 *	Macro assembly instructions require a ; after each line
 */
#define CALL_HANDLER(HANDLER_NAME)\
\
.globl call_##HANDLER_NAME; /* create the asm function call_HANDLER_NAME */\
\
call_ ## HANDLER_NAME ## :;\
	pusha; /* Pushes all registers onto the stack */\
	call HANDLER_NAME; /* calls timer interrupt handler */\
	popa; /* Restores all registers onto the stack */\
	iret; /* Return to procedure before interrupt */

