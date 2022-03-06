/** @file console_driver.c
 *  @brief Contains console driver functions implementint the p1kern interface,
 *         and other helper functions not in the interface as well.
 *
 *  For interface functions as declared in p1kern.h, additional comment blocks
 *  are preceeded and succeeded by '--'.
 *
 *  @author Nicklaus Choo (nchoo)
 *  @bug No known bugs
 */

#include <limits.h> /* INT_MAX */
#include <ctype.h> /* isprint() */
#include <p1kern.h> /* client interface functions */
#include <stddef.h> /* NULL */
#include <assert.h> /* assert() */
#include <x86/asm.h> /* outb() */
#include <string.h> /* memmove () */

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

/*********************************************************************/
/*                                                                   */
/* Internal helper functions                                         */
/*                                                                   */
/*********************************************************************/

/** @brief Checks if row and col are within console screen dimensions.
 *
 *  @param row Row to check
 *  @param col Col to check
 *  @return value greater than 1 if true, 0 otherwise
 */
int
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
void
set_hardware_cursor(int row, int col) {

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

  return;
}

/** @brief Scrolls the terminal up by 1 line and ensures that the position of
 *         the logical cursor remains fixed with respect to console output
 *
 *  @return Void.
 */
void
scroll(void) {

  /* Move screen contents all up 1 row. */
  memmove((void *) CONSOLE_MEM_BASE,
          (void *) (CONSOLE_MEM_BASE + 2 * CONSOLE_WIDTH),
          2 * CONSOLE_WIDTH * (CONSOLE_HEIGHT - 1));

  /* The new last row should be an empty row of spaces */
  for (size_t col = 0; col < CONSOLE_WIDTH; col++) {
    draw_char(CONSOLE_HEIGHT - 1, col, ' ', console_color);
  }
  /* Cursor is stationary relative to output. */
  set_cursor(cursor_row - 1, cursor_col);
}

/** @brief Modification of the putbyte() specification to record if there was
 *         a screen scroll of 1 line after we put a byte on the screen.
 *
 *  putbyte() is a wrapper around this function. Useful for readline() in
 *  the keyboard driver.
 *
 *  @param ch The character to print
 *  @param scrolled Must be 0. Set to 1 if scrolled a line, unchanged otherwise
 *  @return The input character
 */
int
_putbyte(char ch, int *scrolled) {
  assert(scrolled != NULL);
  assert(*scrolled == 0);
  assert(onscreen(cursor_row, cursor_col));

  /* Get current cursor position */
  char ogch = ch;
  int row, col;
  get_cursor(&row, &col);

  switch (ch) {
    case '\n': {

      /* Scroll if at screen bottom */
      if (cursor_row + 1 >= CONSOLE_HEIGHT) {
        scroll();
        *scrolled = 1;
      }
      /* Always update the cursor position relative to content */
      set_cursor(cursor_row + 1, 0);
      break;
    }
    case '\r': {
      set_cursor(cursor_row, 0);
      break;
    }
    case '\b': {

      /* Not at leftmost column */
      if (cursor_col != 0) {

        /* Always draw space before moving else cursor will be blotted out */
        draw_char(cursor_row, cursor_col - 1, ' ', console_color);
        set_cursor(cursor_row, cursor_col - 1);

      /* At leftmost column, backspace goes to previous row */
      } else {
        draw_char(cursor_row - 1, CONSOLE_WIDTH - 1, ' ', console_color);
        set_cursor(cursor_row - 1, CONSOLE_WIDTH - 1);
      }
      break;
    }
    default: {

      /* Print the character, unprintable characters return -1 */
      if (!isprint(ch)) return -1;
      draw_char(cursor_row, cursor_col, ch, console_color);

      /* If we are at the end of a line, set cursor on new line */
      if (cursor_col + 1 >= CONSOLE_WIDTH) {

        /* Scroll if necessary */
        if (cursor_row + 1 >= CONSOLE_HEIGHT) {
          scroll();
          *scrolled = 1;
        }
        /* Start printing below */
        set_cursor(cursor_row + 1, 0);
      } else {
        set_cursor(cursor_row, cursor_col + 1);
      }
      break;
    }
  }
  assert(onscreen(row, col));
  return ogch;
}

/*********************************************************************/
/*                                                                   */
/* Console interface functions                                       */
/*                                                                   */
/*********************************************************************/

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
 *  Unprintable characters return -1.
 *  We move the cursors as we write.
 *  --
 *  @param ch the character to print
 *  @return The input character
 */
int putbyte(char ch) {

  /* Wrapper for _putbyte(). We don't care in putbyte() if we scrolled */
  int scrolled = 0;
  return _putbyte(ch, &scrolled);
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
  if (len < 0 || s == NULL) return;
  for (size_t i = 0; i < len; i++) {
    char ch = s[i];
    putbyte(ch);
  }
}

/** @brief Prints character ch with the specified color
 *         at position (row, col).
 *
 *  If any argument is invalid, the function has no effect.
 *  --
 *  Note that we only draw printable characters. To draw something is to make
 *  it to be seen. If a character cannot be seen/printed it cannot be
 *  drawn and thus we do not draw unprintable characters
 *  --
 *  @param row The row in which to display the character.
 *  @param col The column in which to display the character.
 *  @param ch The character to display.
 *  @param color The color to use to display the character.
 *  @return Void.
 */
void draw_char(int row, int col, int ch, int color) {

  /* If row out of range, invalid row, no effect. */
  if (!(0 <= row && row < CONSOLE_HEIGHT)) return;

  /* If col out of range, invalid col, no effect. */
  if (!(0 <= col && col < CONSOLE_WIDTH)) return;

  /* If ch not printable, invalid ch, no effect. */
  if (!isprint(ch)) return;

  /* If background color not supported, invalid color, no effect. */
  if (color & INVALID_COLOR) return;

  /* All arguments valid, draw character */
  *(char *)(CONSOLE_MEM_BASE + 2*(row * CONSOLE_WIDTH + col)) = ch;
  *(char *)(CONSOLE_MEM_BASE + 2*(row * CONSOLE_WIDTH + col) + 1) = color;
}

/** @brief Returns the character displayed at position (row, col).
 *  @param row Row of the character.
 *  @param col Column of the character.
 *  @return The character at (row, col).
 */
char get_char(int row, int col) {

  /* If out of range, return '\0'. */
  if (!(0 <= row && row < CONSOLE_HEIGHT)
      || !(0 <= col && col < CONSOLE_WIDTH)) {
    return '\0';
  }
  /* Else return char at row, col. */
  return *(char *)(CONSOLE_MEM_BASE + 2*(row * CONSOLE_WIDTH + col));
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
 *  Else, the logical cursor and the hardware cursor are set.
 *  --
 *  @param row The new row for the cursor.
 *  @param col The new column for the cursor.
 *  @return 0 on success or integer error code less than 0 if
 *          cursor location is invalid.
 */
int set_cursor(int row, int col) {

  /* set logical cursor */
  if (onscreen(row, col)) {
    cursor_row = row;
    cursor_col = col;

    /* If cursor is not hidden, set the hardware cursor */
    if (!cursor_hidden) set_hardware_cursor(row, col);
    return 0;
  }
  /* cursor location is invalid, do nothing and return -1 */
  return -1;
}

/** @brief Writes the current position of the cursor
 *         into the arguments row and col.
 *
 *  --
 *  Only writes to row, col if they are non-null
 *  --
 *  @param row The address to which the current cursor
 *         row will be written.
 *  @param col The address to which the current cursor
 *         column will be written.
 *  @return Void.
 */
void get_cursor(int* row, int* col) {
  if (row != NULL) *row = cursor_row;
  if (col != NULL) *col = cursor_col;
  return;
}

/** @brief Shows the cursor.
 *
 *  If the cursor is already shown, the function has no effect.
 *  --
 *  Hides the cursor by setting the hardware cursor to
 *  (CONSOLE_HEIGHT, CONSOLE_WIDTH) and toggles cursor_hidden to true i.e. 1.
 *  Note that this function is idempotent.
 *  --
 *  @return Void.
 */
void hide_cursor(void) {
  assert(onscreen(cursor_row, cursor_col));
  set_hardware_cursor(CONSOLE_HEIGHT, CONSOLE_WIDTH);
  cursor_hidden = 1;
  return;
}

/** @brief Shows the cursor.
 *
 *  If the cursor is already shown, the function has no effect.
 *  --
 *  Shows the cursor by setting the hardware cursor to the current location
 *  of the logical cursor and toggles cursor_hidden to false i.e. 0.
 *  Note that this function is idempotent.
 *  --
 *  @return Void.
 */
void show_cursor(void) {
  assert(onscreen(cursor_row, cursor_col));
  set_hardware_cursor(cursor_row, cursor_col);
  cursor_hidden = 0;
  return;
}

/** @brief Writes the current foreground and background
 *         color of characters printed on the console
 *         into the argument color.
 *  @param color The address to which the current color
 *         information will be written.
 *  @return Void.
 */
void get_term_color(int* color) {
  if (color != NULL) *color = console_color;
}

/** @brief Changes the foreground and background color
 *         of future characters printed on the console.
 *
 *  If the color code is invalid, the function has no effect.
 *
 *  @param color The new color code.
 *  @return 0 on success or integer error code less than 0 if
 *          color code is invalid.
 */
int set_term_color(int color) {

  /* No effect if invalid color passed */
  if (color & INVALID_COLOR) return -1;

  /* Else set console_color */
  console_color = color;
  return 0;
}

/** @brief Clears the entire console.
 *
 * The cursor is reset to the first row and column
 *
 *  @return Void.
 */
void clear_console(void) {
  for (size_t row = 0; row < CONSOLE_HEIGHT; row++) {
    for (size_t col = 0; col < CONSOLE_WIDTH; col++) {
      draw_char(row, col, ' ', console_color);
    }
  }
  /* Set cursor to the top left corner */
  set_cursor(0, 0);
}


