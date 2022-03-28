/** @file asm_interrupt_handler.S
 *  @brief Implementations for assembly functions
 *  @author Nicklaus Choo (nchoo)
 *  @bugs No known bugs
 */

#include <asm_interrupt_handler_template.h>

CALL_HANDLER(timer_int_handler)

CALL_HANDLER(keybd_int_handler)

CALL_W_SINGLE_ARG(test_int_handler)
