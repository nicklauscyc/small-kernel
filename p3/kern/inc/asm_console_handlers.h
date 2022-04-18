/** @file asm_console_handlers.h
 *  @brief Assembly wrapper to call console syscall handlers
 */

#ifndef ASM_CONSOLE_HANDLERS_H_
#define ASM_CONSOLE_HANDLERS_H_

void call_print(void);
void call_readline(void);
void call_get_cursor_pos(void);
void call_set_cursor_pos(void);
void call_set_term_color_handler(void);

#endif /* ASM_CONSOLE_HANDLERS_H_ */

