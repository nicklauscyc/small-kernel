/** @file asm_thread_management_handlers.h
 *  @brief Assembly wrapper to call syscall handlers
 *
 *  @author Nicklaus Choo (nchoo)
 */

#ifndef ASM_THREAD_MANAGEMENT_HANDLERS_H_
#define ASM_THREAD_MANAGEMENT_HANDLERS_H_

extern void call_gettid( void );

extern void call_get_ticks( void );

extern void call_yield( void );

extern void call_deschedule( void );

extern void call_make_runnable( void );

extern void call_sleep( void );

#endif /* ASM_THREAD_MANAGEMENT_HANDLERS_H_ */
