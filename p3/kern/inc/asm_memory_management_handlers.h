/** @file asm_memory_management_handlers.h
 *  @brief Assembly wrapper to call memory management syscall handlers
 *
 *  @author Nicklaus Choo (nchoo)
 */

#ifndef ASM_MEMORY_MANAGEMENT_HANDLERS_H_
#define ASM_MEMORY_MANAGEMENT_HANDLERS_H_

void call_pagefault_handler( void );

void call_new_pages( void );

void call_remove_pages( void );

#endif /* ASM_MEMORY_MANAGEMENT_HANDLERS_H_ */

