/** @file keyboard.c
 *  @brief Contains functions that help the user type into the console
 *
 *  @author Nicklaus Choo (nchoo)
 *  @bug No known bugs
 */

#include <x86/keyhelp.h> /* process_scancode() */
#include <x86/video_defines.h> /* CONSOLE_HEIGHT, CONSOLE_WIDTH */
#include <assert.h> /* assert() */
#include <string.h> /* memcpy() */
#include <x86/asm.h> /* process_scancode() */
#include <x86/interrupt_defines.h> /* INT_CTL_PORT */
#include "inc/console.h" /* console functions */
#include "inc/buffer.h" /* buffer_t functions */

/* Keyboard buffer */
static buffer_t key_buf;

typedef int aug_char;

/** @brief Interrupt handler which reads in raw bytes from keystrokes. Reads
 *         incoming bytes to the keyboard buffer key_buf, which has an
 *         amortized constant time complexity for adding elements. So it
 *         returns quickly
 *
 *  @return Void.
 */
void keybd_int_handler(void) {

  /* Read raw byte and put into raw character buffer */
  buffer_add(&key_buf, inb(KEYBOARD_PORT));

  /* Acknowledge interrupt and return */
  outb(INT_CTL_PORT, INT_ACK_CURRENT);
}

/** @brief Initialize the keyboard interrupt handler and associated data
 *         structures
 *
 *  Memory for keybd_buf is allocated here.
 *
 *  @return Void.
 */
void init_keybd(void) {
  init_buffer(&key_buf);
  assert(is_buffer(&key_buf);
}

/*********************************************************************/
/*                                                                   */
/* Keyboard driver interface                                         */
/*                                                                   */
/*********************************************************************/

/** @brief Returns the next character in the keyboard buffer
 *
 *  This function does not block if there are no characters in the keyboard
 *  buffer
 *  --
 *  No other process will call readchar() concurrently with readline() since
 *  we only have 1 kernal process running and have a single thread.
 *  --
 *  @return The next character in the keyboard buffer, or -1 if the keyboard
 *          buffer is currently empty
 **/
int readchar(void) {

	disable_interrupts();

	/* Keep getting the next character while buffer is not empty */
	while (!buffer_empty(&key_buf)) {
		uint8_t next_byte = (uint8_t) buffer_rem(&key_buf);
		enable_interrupts();

		/* Get augmented character */
		aug_char next_char = process_scancode(next_byte);

		/* Get simplified character */
		if (KH_HASDATA(next_char) && KH_ISMAKE(next_char)) {
			unsigned char next_char_value = KH_GETCHAR(next_char);
			return (int) (unsigned int) next_char_value;
		}
		continue;
	}
	return -1;
}

/** @brief Reads a line of characters into a specified buffer
 *
 * If the keyboard buffer does not already contain a line of input,
 * readline() will spin until a line of input becomes available.
 *
 * If the line is smaller than the buffer, then the complete line,
 * including the newline character, is copied into the buffer.
 *
 * If the length of the line exceeds the length of the buffer, only
 * len characters should be copied into buf.
 *
 * Available characters should not be committed into buf until
 * there is a newline character available, so the user has a
 * chance to backspace over typing mistakes.
 *
 * While a readline() call is active, the user should receive
 * ongoing visual feedback in response to typing, so that it
 * is clear to the user what text line will be returned by
 * readline().
 * --
 * the definition of a line in readline() is different from a row. A
 * carriage-return will return the cursor to its initial position at the
 * start of the call to readline(). Backspaces will always work and only
 * do nothing if the cursor is at the initial position at the start of the
 * call to readline.
 *
 * Since it is only meaningful that the user can see exactly what was written
 * to buf, If len is valid the moment the user types len bytes readline() will
 * return. We prevent the user from typing more than len characters into the
 * console.
 * --
 *  @param buf Starting address of buffer to fill with a text line
 *  @param len Length of the buffer
 *  @return The number of characters in the line buffer,
 *          or -1 if len is invalid or unreasonably large.
 */
int readline(char *buf, int len) {

  /* buf == NULL so invalid buf */
  if (!buf) return -1;

  /* len < 0 so invalid len */
  if (len < 0) return -1;

  /* len == 0 so no need to copy */
  if (len == 0) return 0;

  /* get original cursor position for start of line relative to scroll */
  int start_row, start_col;
  get_cursor(&start_row, &start_col);

  /* Allocate space for temporary buffer */
  char *temp_buf = calloc(len, sizeof(char));

  /* len > 0 but unreasonably large since calloc failed, so return -1 */
  if (temp_buf == NULL) return -1;

  /* Initialize index into temp_buf */
  int i = 0;
  int written = 0; /* characters written so far */
  char ch;

  while ((ch = get_next_char()) != '\n' && written < len) {

    /* ch, i, written is always in range */
    assert(0 <= i && i < len);
    assert(0 <= written && written < len);

    /* tracks if scrolling occured or not */
    int scrolled = 0;

    /* If at front of buffer, Delete the character if backspace */
    if (ch == '\b') {

      /* If at start_row, start_col, do nothing as don't delete prompt */
      int row, col;
      get_cursor(&row, &col);
      assert (row * CONSOLE_WIDTH + col >= start_row * CONSOLE_WIDTH + start_col);
      if (!(row == start_row && col == start_col)) {
        assert(i > 0);

        /* Print to screen and update intial cursor position if needed*/
        scrolled_putbyte(ch, &start_row, &start_col);
        if (scrolled) start_row -= 1;

        /* update i and buffer */
        i--;
        temp_buf[i] = ' ';
      }
    /* '\r' sets cursor to position at start of call. Don't overwrite prompt */
    } else if (ch == '\r') {

      /* Set cursor to start of line w.r.t start of call, i to buffer start */
      set_cursor(start_row, start_col);
      i = 0;

    /* Regular characters just write, unprintables do nothing */
    } else {

      /* print on screen and update initial cursor position if needed */
      scrolled_putbyte(ch, &start_row, &start_col);
      if (scrolled) start_row -= 1;

      /* write to buffer */
      if (isprint(ch)) {
        temp_buf[i] = ch;
        i++;
        if (i > written) written = i;
      }
    }
  }
  assert(written <= len);
  if (ch == '\n') {

    /* Only write the newline if there's space for it in the buffer */
    if (written < len) {
      putbyte(ch);
      temp_buf[i] = '\n';
      i++;
      if (i > written) written = i;
    }
  } else {
    assert(written == len);
  }
  memcpy(buf, temp_buf, written);
  free(temp_buf);
  return written;
}



