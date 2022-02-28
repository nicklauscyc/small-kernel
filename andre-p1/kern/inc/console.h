/** @file console.h
 *  @brief Function prototypes for the console driver.
 *
 *  @author Andre Nascimento (anascime)
 *  @bug No known bugs.
 *  */

#ifndef _CONSOLE_H_
#define _CONSOLE_H_

int putbyte( char ch );

void putbytes( const char *s, int len );

int set_term_color( int color );

void get_term_color( int *color );

int set_cursor( int row, int col );

void get_cursor( int *row, int *col );

void hide_cursor(void);

void show_cursor(void);

void clear_console(void);

void draw_char( int row, int col, int ch, int color );

char get_char( int row, int col );

#endif /* _CONSOLE_H_ */
