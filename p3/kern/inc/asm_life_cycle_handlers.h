/** @file asm_life_cycle_handlers.h
 *  @brief Assembly wrapper to call life cycle syscall handlers
 *
 *  @author Nicklaus Choo (nchoo)
 */

#ifndef ASM_LIFE_CYCLE_HANDLERS_H_
#define ASM_LIFE_CYCLE_HANDLERS_H_

extern void call_fork( void );
extern void call_exec( void );
extern void call_vanish( void );
extern void call_task_vanish( void );
extern void call_set_status( void );
extern void call_wait( void );

#endif /* ASM_LIFE_CYCLE_HANDLERS_H_ */
