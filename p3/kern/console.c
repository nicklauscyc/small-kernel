/** @file console.c
 *	@brief Implements functions in console.h
 *
 *	@author Andre Nascimento (anascime)
 *	@author Nicklaus Choo (nchoo)
 */

#include <console.h>

/* Background color mask to extract invalid set bits in color. FFFF FF00 */
#define INVALID_COLOR 0xFFFFFF00



//
//
///** TODO everything below here has been melded together from andre's and nick's
// *  code
// */
///** @brief Clears the entire console.
// *
// * The cursor is reset to the first row and column
// *
// *  @return Void.
// */
//void clear_console( void )
//{
//	for (size_t row = 0; row < CONSOLE_HEIGHT; row++) {
//		for (size_t col = 0; col < CONSOLE_WIDTH; col++) {
//			draw_char(row, col, ' ', console_color);
//		}
//	}
//	/* Set cursor to the top left corner */
//	set_cursor(0, 0);
//}
//
///** @brief Prints character ch with the specified color
// *		   at position (row, col).
// *
// *	If any argument is invalid, the function has no effect.
// *
// *	@param row The row in which to display the character.
// *	@param col The column in which to display the character.
// *	@param ch The character to display.
// *	@param color The color to use to display the character.
// *	@return Void.
// */
//void
//draw_char( int row, int col, int ch, int color )
//{
//	/* If row or col out of range, invalid row, no effect. */
//	if (!(0 <= row && row < CONSOLE_HEIGHT)) return;
//	if (!(0 <= col && col < CONSOLE_WIDTH)) return;
//
//	/* If background color not supported, invalid color, no effect. */
//	if (color & INVALID_COLOR) return;
//
//	/* All arguments valid, draw character with specified color */
//	char *chp = *(char *)(CONSOLE_MEM_BASE + 2*(row * CONSOLE_WIDTH + col));
//	*chp = ch;
//	*(chp + 1) = color;
//}
//
///** @brief Returns the character displayed at position (row, col).
// *
// *  --
// *  Offscreen characters are always NULL or simply 0 by default.
// *  --
// *
// *	@param row Row of the character.
// *	@param col Column of the character.
// *	@return The character at (row, col).
// */
//char
//get_char( int row, int col )
//{
//	/* If out of range, return 0. */
//	if (!(0 <= row && row < CONSOLE_HEIGHT) return 0;
//	if (!(0 <= col && col < CONSOLE_HEIGHT) return 0;
//
//	/* Else return char at row, col. */
//	return *(char *)(CONSOLE_MEM_BASE + 2*(row * CONSOLE_WIDTH + col));
//}

