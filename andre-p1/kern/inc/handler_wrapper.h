/** @file handler_wrapper.h
 *  @brief Function prototypes for asm handler wrappers.
 *
 *  @author Andre Nascimento (anascime)
 *  @bug No known bugs.
 *  */

#ifndef HANDLER_WRAPPER_H_
#define HANDLER_WRAPPER_H_

/** @brief Wrapper for timer handler.
 *
 *  @return Void.
 *  */
void asm_timer_wrapper(void);

/** @brief Wrapper for keyboard handler.
 *
 * @return Void.
 * */
void asm_keyboard_wrapper(void);

#endif /* HANDLER_WRAPPER_H_ */
