/** @file asm_interrupt_handler.h
 *
 *  @brief helper functions to save register values before calling C
 *         interrupt handler functions, and then restore registers and return
 *         to normal execution.
 *
 */

#ifndef _P1_ASM_INTERRUPT_HANDLER_H_
#define _P1_ASM_INTERRUPT_HANDLER_H_

#include "./timer_driver.h"
#include "./keybd_driver.h"

/** @brief Saves all register values, calls timer_int_handler(), restores
 *         all register values and returns to normal execution.
 *
 *  pusha is used to save all registers. Though not needed, %esp is also pushed
 *  onto the stack for convenience of implentation. A call is made to
 *  timer_int_handler(). When timer_int_handler() returns, popa is used
 *  to restore all registers in the correct order, and iret to return to
 *  normal execution prior to the interrupt.
 *
 *  @return Void.
 */
void call_timer_int_handler(void);

/** @brief Saves all register values, calls keybd_int_handler(), restores
 *         all register values and returns to normal execution.
 *
 *  pusha is used to save all registers. Though not needed, %esp is also pushed
 *  onto the stack for convenience of implentation. A call is made to
 *  keybd_int_handler(). When keybd_int_handler() returns, popa is used
 *  to restore all registers in the correct order, and iret to return to
 *  normal execution prior to the interrupt.
 *
 *  @return Void.
 */
void call_keybd_int_handler(void);

#endif
