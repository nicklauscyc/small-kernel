/** @file install_thread_management_handlers.h
 *  @brief Functions to install thread management syscall handlers
 *  @author Nicklaus Choo (nchoo)
 *
 */

#ifndef _INSTALL_THREAD_MANAGEMENT_HANDLERS_H_
#define _INSTALL_THREAD_MANAGEMENT_HANDLERS_H_

/** @brief Assembly wrapper for calling gettid() interrupt handler
 */
void call_gettid_int_handler(void);
int install_gettid_handler(int idt_entry, asm_wrapper_t *asm_wrapper);
#endif /* _INSTALL_THREAD_MANAGEMENT_HANDLERS_H_ */

