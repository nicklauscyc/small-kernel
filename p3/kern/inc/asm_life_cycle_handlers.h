/** @file asm_life_cycle_handlers.h
 *  @brief Assembly wrapper to call life cycle syscall handlers
 *
 *  @author Nicklaus Choo (nchoo)
 */

#ifndef ASM_LIFE_CYCLE_HANDLERS_H_
#define ASM_LIFE_CYCLE_HANDLERS_H_

extern void call_fork( void );
extern void call_exec( char *execname, char **argvec );

#endif /* ASM_LIFE_CYCLE_HANDLERS_H_ */
