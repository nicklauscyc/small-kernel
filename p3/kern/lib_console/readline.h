/** @file Readline definitions for others to use
 *
 *  @brief Exports read_char_arrived handler, essential
 *  for not wasting cycles checking if new characters
 *  arrived inside the readline syscall.
 *  */

#ifndef READLINE_H_
#define READLINE_H_

void init_readline( void );
void readline_char_arrived_handler( void );

#endif /* READLINE_H_ */
