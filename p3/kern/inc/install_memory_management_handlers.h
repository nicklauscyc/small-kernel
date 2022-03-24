/** @file install_memory_management_handlers.h
 *  @brief Functions to install memory management syscall handlers
 *
 *  @author Nicklaus Choo (nchoo)
 */

#ifndef _INSTALL_MEMORY_MANAGEMENT_HANDLERS_H_
#define _INSTALL_MEMORY_MANAGEMENT_HANDLERS_H_

/** @brief Assembly wrapper for calling gettid() interrupt handler
 */
void call_pagefault_handler(void);
int install_pf_handler(int idt_entry, asm_wrapper_t *asm_wrapper);
#endif /* _INSTALL_MEMORY_MANAGEMENT_HANDLERS_H_ */

