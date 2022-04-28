/** @file context_switch.h */

#ifndef CONTEXT_SWITCH_H_
#define CONTEXT_SWITCH_H_

/** @brief Switches between two threads, saving the current
 *		   thread's registers and updating current register's
 *		   to the new thread's.
 *
 *	@pre restore_esp stack has been setup by either a prior context switch
 *		 (where it was save_esp) or by save_child_regs.
 *	@param save_esp Pointer to stack where to store registers
 *	@param restore_esp Pointer to stack where to find new registers
 *
 *	@return Returns with new context (ie doesn't return in C sense) */
void context_switch( void **save_esp, void *restore_esp );

#endif /* CONTEXT_SWITCH_H_ */

