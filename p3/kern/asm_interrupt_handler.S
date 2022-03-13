/** @file asm_interrupt_handler.S
 *  @brief Implementations for assembly functions
 *  @author Nicklaus Choo (nchoo)
 *  @bugs No known bugs
 */

#include <asm_interrupt_handler_template.h>

CALL_HANDLER(timer_int_handler)

//.globl call_timer_int_handler
//
//call_timer_int_handler:
//  pusha /* Pushes all registers onto the stack */
//  call timer_int_handler /* calls timer interrupt handler */
//  popa /* Restores all registers onto the stack */
//  iret /* Return to procedure before interrupt */

CALL_HANDLER(keybd_int_handler)

//.globl call_keybd_int_handler
//
//call_keybd_int_handler:
//  pusha /* Pushes all registers onto the stack */
//  call keybd_int_handler /* calls timer interrupt handler */
//  popa /* Restores all registers onto the stack */
//  iret /* Return to procedure before interrupt */

