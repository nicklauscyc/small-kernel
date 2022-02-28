/** @file keyboard.h
 *  @brief Function prototypes for keyboard functions.
 *
 *  @author Andre Nascimento (anascime)
 *  @bug No known bugs.
 *  */

#ifndef _KEYBOARD_H_
#define _KEYBOARD_H_

void setup_keyboard(void);

int readchar(void);

int readline(char *buf, int len);

#endif /* _KEYBOARD_H_ */
