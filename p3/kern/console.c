/** @file console.c
 *	@brief Implements functions in console.h
 *
 *	Since console.h contains comments, added comments will be written
 *	with this format with preceding and succeeding --
 *
 *	@author Andre Nascimento (anascime)
 *	@author Nicklaus Choo (nchoo)
 */

#include <console.h>
#include <asm.h>			/* outb() */
#include <string.h>			/* memmove () */
#include <assert.h>			/* assert(), affirm() */
#include <video_defines.h>	/* CONSOLE_HEIGHT, CONSOLE_WIDTH */
#include <lib_thread_management/mutex.h> /* mutex_t */

/** @brief Mutex for drawing chars */
static mutex_t draw_char_mux;

/** @brief Mutex for updating/reading cursor */
static mutex_t cursor_mux;

/* Default global console color. */
static int console_color = BGND_BLACK | FGND_WHITE;

/* Logical cursor row starts off at 0 */
static int cursor_row = 0;

/* Logical cursor col starts off at 0 */
static int cursor_col = 0;

/* Boolean for if cursor is hidden */
static int cursor_hidden = 0;

/* Background color mask to extract invalid set bits in color. FFFF FF00 */
#define INVALID_COLOR 0xFFFFFF00

/** @brief Initializes the console
 *
 *  @return Void. */
void
init_console( void )
{
	mutex_init(&draw_char_mux);
	mutex_init(&cursor_mux);
	clear_console();
}


/** @brief Helper function to check if (row, col) is onscreen
 *
 * 	@param row Row index
 * 	@param col Col index
 * 	@return 1 if onscreen, 0 otherwise
 */
static int
onscreen(int row, int col) {
	return 0 <= row && row < CONSOLE_HEIGHT && 0 <= col && col < CONSOLE_WIDTH;
}

/** @brief Sets the hardware cursor to any row or column. Should only be called
 *         by hide_cursor(), unhide_cursor(), set_cursor().
 *
 *  Invariant for row and col is such that the cursor is only ever set to
 *  be onscreen, or offscreen specifically at (CONSOLE_HEIGHT, CONSOLE_WIDTH).
 *
 *  @param row Row to set hardware cursor to.
 *  @param col Column to set hardware cursor to.
 *  @return Void.
 */
static void
set_hardware_cursor( int row, int col )
{
	/* Only values for row, col if called by hide, unhide, set cursor functions */
	assert(onscreen(row, col) || (row == CONSOLE_HEIGHT && col == CONSOLE_WIDTH));

	/* Calculate offset in row major form */
	short hardware_cursor_offset = row * CONSOLE_WIDTH + col;

	/* Set lower 8 bits */
	outb(CRTC_IDX_REG, CRTC_CURSOR_LSB_IDX);
	outb(CRTC_DATA_REG, hardware_cursor_offset);

	/* Set upper 8 bits */
	outb(CRTC_IDX_REG, CRTC_CURSOR_MSB_IDX);
	outb(CRTC_DATA_REG, hardware_cursor_offset >> 8);
}

/** @brief Scrolls the terminal up by 1 line and ensures that the position of
 *         the logical cursor remains fixed with respect to console output
 *
 *  @return Void.
 */
static void
scroll( void )
{
	/* Move screen contents all up 1 row. */
	memmove((void *) CONSOLE_MEM_BASE,
			(void *) (CONSOLE_MEM_BASE + 2 * CONSOLE_WIDTH),
			2 * CONSOLE_WIDTH * (CONSOLE_HEIGHT - 1));

	/* The new last row should be an empty row of spaces */
	for (int col = 0; col < CONSOLE_WIDTH; col++) {
		draw_char(CONSOLE_HEIGHT - 1, col, ' ', console_color);
	}
	/* Cursor is stationary relative to output. */
	set_cursor(cursor_row - 1, cursor_col);
}

/** @brief Modification of the putbyte() specification which takes in
 *         arguments for starting row and column. Useful for readline()
 *
 *  putbyte() is a wrapper around this function.
 *
 *  @param ch The character to print
 *  @param start_rowp Pointer to starting row
 *  @param start_colp Pointer to starting column
 *  @return The input character
 */
int
scrolled_putbyte( char ch, int *start_rowp, int *start_colp )
{
	assert(start_rowp);
	assert(start_colp);
	assert(onscreen(*start_rowp, *start_colp));
	assert(onscreen(cursor_row, cursor_col));

	switch (ch) {
		case '\n': {

			/* Scroll if at screen bottom */
			if (cursor_row + 1 >= CONSOLE_HEIGHT) {
				scroll();
				*start_rowp -= 1;
			}
			/* Always update the cursor position relative to content */
			draw_char(cursor_row + 1, 0, ' ', console_color);
			set_cursor(cursor_row + 1, 0);
			break;
		}
		case '\r': {
			set_cursor(*start_rowp, *start_colp);
			break;
		}
		case '\b': {

			/* Not at leftmost column */
			if (cursor_col > 0) {

			  /* Always draw space before moving else cursor blotted out */
			  draw_char(cursor_row, cursor_col - 1, ' ', console_color);
			  set_cursor(cursor_row, cursor_col - 1);

			/* At leftmost column, backspace goes to previous row if not top */
			} else {
				if (cursor_row > 0) {
					draw_char(cursor_row - 1, CONSOLE_WIDTH - 1, ' ',
					          console_color);
					set_cursor(cursor_row - 1, CONSOLE_WIDTH - 1);
				}
			}
			break;
		}
		default: {

			/* Print the character */
			draw_char(cursor_row, cursor_col, ch, console_color);

			/* If we are at the end of a line, set cursor on new line */
			if (cursor_col + 1 >= CONSOLE_WIDTH) {

				/* Scroll if necessary */
				if (cursor_row + 1 >= CONSOLE_HEIGHT) {
					scroll();
					*start_rowp -= 1;
				}
				/* Start printing below, update color if needed */
				char next_ch = get_char(cursor_row + 1, 0);
				draw_char(cursor_row + 1, 0, next_ch, console_color);
				set_cursor(cursor_row + 1, 0);
			} else {
				char next_ch = get_char(cursor_row, cursor_col + 1);
				draw_char(cursor_row, cursor_col + 1, next_ch, console_color);
				set_cursor(cursor_row, cursor_col + 1);
			}
			break;
		}
	}
	assert(onscreen(cursor_row, cursor_col));
	return ch;
}

/** @brief Prints character ch at the current location
 *         of the cursor.
 *
 *  If the character is a newline ('\n'), the cursor is moved
 *  to the beginning of the next line (scrolling if necessary).
 *  If the character is a carriage return ('\r'), the cursor is
 *  immediately reset to the beginning of the current line,
 *  causing any future output to overwrite any existing output
 *  on the line.  If backspace ('\b') is encountered, the previous
 *  character is erased.  See the main console.c description found
 *  on the handout web page for more backspace behavior.
 *  --
 *  We move the cursors as we write. Since there is no notion of a console
 *  prompt for putbyte(), the call to scrolled_putbyte() will have
 *  start_row == cursor_row and start_col == 0
 *  --
 *
 *  @param ch the character to print
 *  @return The input character
 */
int putbyte(char ch) {

	/* Get starting row and column, but set starting column to 0 */
	int start_row;
	int start_col;
	get_cursor(&start_row, &start_col);
	start_col = 0;

	return scrolled_putbyte(ch, &start_row, &start_col);
}
/** @brief Prints the string s, starting at the current
 *         location of the cursor.
 *
 *  If the string is longer than the current line, the
 *  string fills up the current line and then
 *  continues on the next line. If the string exceeds
 *  available space on the entire console, the screen
 *  scrolls up one line, and then the string
 *  continues on the new line.  If '\n', '\r', and '\b' are
 *  encountered within the string, they are handled
 *  as per putbyte. If len is not a positive integer or s
 *  is null, the function has no effect.
 *
 *  @param s The string to be printed.
 *  @param len The length of the string s.
 *  @return Void.
 */
void
putbytes( const char *s, int len )
{
	affirm(s);
	if (len < 0) {
		return;
	}
	/* s is null string doesn't mean s == NULL */
	if (s[0] == '\0') {
		return;
	}
	for (int i = 0; i < len; i++) {
		char ch = s[i];
		putbyte(ch);
	}
}
/** @brief Changes the foreground and background color
 *         of future characters printed on the console.
 *
 *  If the color code is invalid, the function has no effect.
 *
 *  NOTE: No need for synchronization here, since it's just
 *  a blind write over the current color
 *
 *  @param color The new color code.
 *  @return 0 on success or integer error code less than 0 if
 *          color code is invalid.
 */
int
set_term_color( int color )
{
	/* No effect if invalid color passed */
	if (color & INVALID_COLOR) {
		return -1;
	}
	/* Else set console_color */
	console_color = color;
	return 0;
}

/** @brief Writes the current foreground and background
 *         color of characters printed on the console
 *         into the argument color.
 *  @param color The address to which the current color
 *         information will be written.
 *  @return Void.
 */
void
get_term_color( int* color )
{
	affirm(color);
	*color = console_color;
}

/** @brief Sets the position of the cursor to the
 *         position (row, col).
 *
 *  Subsequent calls to putbytes should cause the console
 *  output to begin at the new position. If the cursor is
 *  currently hidden, a call to set_cursor() does not show
 *  the cursor.
 *  --
 *  If cursor_hidden, the logical cursor is set without setting the
 *  hardware cursor. This is because the hardware cursor is always
 *  the one that is visible.
 *
 *  Else, if there is a change in row, col, the logical cursor and the hardware
 *  cursor are set.
 *  --
 *
 *  @param row The new row for the cursor.
 *  @param col The new column for the cursor.
 *  @return 0 on success or integer error code less than 0 if
 *          cursor location is invalid.
 */
int
set_cursor( int row, int col )
{
	mutex_lock(&cursor_mux);
	assert(onscreen(cursor_row, cursor_col));
	/* set logical cursor */
	if (onscreen(row, col)) {
		cursor_row = row;
		cursor_col = col;

		/* If cursor not hidden and change in row, col, set hardware cursor */
		if (!cursor_hidden && (cursor_row != row || cursor_col != col)) {
			set_hardware_cursor(row, col);
		}
		assert(onscreen(cursor_row, cursor_col));
		mutex_unlock(&cursor_mux);
		return 0;
	}
	/* cursor location is invalid, do nothing and return -1 */
	assert(onscreen(cursor_row, cursor_col));
	mutex_unlock(&cursor_mux);
	return -1;
}

/** @brief Writes the current position of the cursor
 *         into the arguments row and col.
 *
 *  --
 *  Only writes to row, col if they are non-null, throws affirm() error
 *  otherwise.
 *  --
 *
 *  @param row The address to which the current cursor
 *         row will be written.
 *  @param col The address to which the current cursor
 *         column will be written.
 *  @return Void.
 */
void
get_cursor( int* row, int* col )
{
	mutex_lock(&cursor_mux);
	affirm(row);
	affirm(col);
	*row = cursor_row;
	*col = cursor_col;
	mutex_unlock(&cursor_mux);
}

/** @brief Shows the cursor.
 *
 *  If the cursor is already shown, the function has no effect.
 *  --
 *  Hides the cursor by setting the hardware cursor to
 *  (CONSOLE_HEIGHT, CONSOLE_WIDTH) and toggles cursor_hidden to true i.e. 1.
 *  Note that this function is idempotent.
 *  --
 *
 *  @return Void.
 */
void
hide_cursor( void )
{
	assert(onscreen(cursor_row, cursor_col));
	set_hardware_cursor(CONSOLE_HEIGHT, CONSOLE_WIDTH);
	cursor_hidden = 1;
}

/** @brief Shows the cursor.
 *
 *  If the cursor is already shown, the function has no effect.
 *  --
 *  Shows the cursor by setting the hardware cursor to the current location
 *  of the logical cursor and toggles cursor_hidden to false i.e. 0.
 *  Note that this function is idempotent.
 *  --
 *
 *  @return Void.
 */
void
show_cursor( void )
{
	assert(onscreen(cursor_row, cursor_col));
	set_hardware_cursor(cursor_row, cursor_col);
	cursor_hidden = 0;
}


/** @brief Clears the entire console.
 *
 * The cursor is reset to the first row and column
 *
 *  @return Void.
 */
void
clear_console( void )
{
	/* Replace everything onscreen with a blank space */
	for (size_t row = 0; row < CONSOLE_HEIGHT; row++) {
		for (size_t col = 0; col < CONSOLE_WIDTH; col++) {
			draw_char(row, col, ' ', console_color);
		}
	}
	/* Set cursor to the top left corner */
	set_cursor(0, 0);
}

/** @brief Prints character ch with the specified color
 *		   at position (row, col).
 *
 *	If any argument is invalid, the function has no effect.
 *
 *	@param row The row in which to display the character.
 *	@param col The column in which to display the character.
 *	@param ch The character to display.
 *	@param color The color to use to display the character.
 *	@return Void.
 */
void
draw_char( int row, int col, int ch, int color )
{
	mutex_lock(&draw_char_mux);
	/* If row or col out of range, invalid row, no effect. */
	if (!onscreen(row, col)) {
		goto draw_char_cleanup;
	}
	/* If background color not supported, invalid color, no effect. */
	if (color & INVALID_COLOR) {
		goto draw_char_cleanup;
	}
	/* All arguments valid, draw character with specified color */
	char *chp = (char *)(CONSOLE_MEM_BASE + 2*(row * CONSOLE_WIDTH + col));
	*chp = ch;
	*(chp + 1) = color;

draw_char_cleanup:
	mutex_unlock(&draw_char_mux);
}

/** @brief Returns the character displayed at position (row, col).
 *
 *  --
 *  Offscreen characters are always NULL or simply 0 by default.
 *  --
 *
 *	@param row Row of the character.
 *	@param col Column of the character.
 *	@return The character at (row, col).
 */
char
get_char( int row, int col )
{
	/* If out of range, return 0. */
	if (!onscreen(row, col)) {
		return 0;
	}
	/* Else return char at row, col. */
	return *(char *)(CONSOLE_MEM_BASE + 2*(row * CONSOLE_WIDTH + col));
}
