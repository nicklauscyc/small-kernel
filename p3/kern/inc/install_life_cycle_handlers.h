/** @file install_life_cycle_handlers.h
 *  @brief Functions to install thread management syscall handlers
 *
 *  @author Nicklaus Choo (nchoo)
 *
 */

#ifndef _INSTALL_LIFE_CYCLE_HANDLERS_H_
#define _INSTALL_LIFE_CYCLE_HANDLERS_H_

/** @brief Assembly wrapper for calling fork() interrupt handler
 */
void call_fork_int_handler(void);
int install_fork_handler(int idt_entry, asm_wrapper_t *asm_wrapper);
#endif /* _INSTALL_LIFE_CYCLE_HANDLERS_H_ */

