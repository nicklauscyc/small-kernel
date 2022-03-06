/** @file keybd_driver.c
 *  @brief Contains functions that help the user type into the console
 *
 *  @bug No known bugs.
 *
 *
 *  Since unbounded arrays are used, let 'size' be the number of elements in the
 *  array that we care about, and 'limit' be the actual length of the array.
 *  Whenever the size of the array == its limit, the array limit is doubled.
 *  Doubling is only allowed if the current limit <= UINT32_MAX to prevent
 *  overflow.
 *
 *  Indexing into circular arrays is just done modulo the limit of the array.
 *
 *  The two functions in the keyboard driver interface readchar() amd readline()
 *  are closely connected to one another. The specification for readline()
 *  states that "Characters not placed into the specified buffer should remain
 *  available for other calls to readline() and/or readchar()." To put it
 *  concisely, we have the implication:
 *
 *    char not committed to any buffer => char available for other calls
 *
 *    i.e.
 *
 *    char not available for other calls => char committed to some buffer
 *
 *  The question now is under what circumstance do we promise that a char
 *  is not available? It is reasonable to conclude that the above implication
 *  is in fact a bi-implication. Therefore:
 *
 *    char not available for other calls <=> char committed to some buffer
 *
 *  Now since readchar() always reads the next character in the keyboard buffer,
 *  it is then conceivable that if a readchar() not called by readline() takes
 *  the next character off the keyboard buffer, then readline() would "skip"
 *  a character. But then, the specification says that:
 *
 *  "Since we are operating in a single-threaded environment, only one of
 *  readline() or readchar() can be executing at any given point."
 *
 *  Therefore we will never have readline() and readchar() concurrently
 *  executing in seperate threads in the context of the same process, since
 *  when we speak of threads, we refer to threads in the same process.
 *
 *  Therefore we need not worry about the case where a readchar() that is
 *  not invoked by readline() is called in the middle of another call to
 *  readline().
 *
 *  @author Nicklaus Choo (nchoo)
 *  @bug No known bugs
 */

#include <x86/keyhelp.h> /* process_scancode() */
#include <x86/video_defines.h> /* CONSOLE_HEIGHT, CONSOLE_WIDTH */
#include <malloc.h> /* calloc */
#include <stddef.h> /* NULL */
#include <assert.h> /* assert() */
#include <console.h> /* putbyte() */
#include <string.h> /* memcpy() */
#include <x86/asm.h> /* process_scancode() */
#include <x86/interrupt_defines.h> /* INT_CTL_PORT */
#include <ctype.h> /* isprint() */
#include "./console_driver.h" /* _putebyte() */
#include "./keybd_driver.h" /* uba */

/* Keyboard buffer */
static uba *key_buf = NULL;

int readchar(void);

/*********************************************************************/
/*                                                                   */
/* Internal helper functions                                         */
/*                                                                   */
/*********************************************************************/

/** @brief Checks invariants for unbounded arrays
 *
 *  The invariants here are implemented as asserts so in the even that
 *  an assertion fails, the developer knows exactly which assertion
 *  fails. There are many invariants since on top of being an
 *  unbounded array, it is also a circular array.
 *
 *  @param arr Pointer to uba to be checked
 *  @return 1 if valid uba pointer, 0 otherwise
 */
int is_uba(uba *arr) {

  /* non-NULL check */
  assert(arr != NULL);

  /* size check */
  assert(0 <= arr->size && arr->size < arr->limit);

  /* limit check */
  assert(0 < arr->limit);

  /* first and last check */
  if (arr->first <= arr->last) {
    assert(arr->size == arr->last - arr->first);
  } else {
    assert(arr->size == arr->limit - arr->first + arr->last);
  }
  /* first and last in bounds check */
  assert(0 <= arr->first && arr->first < arr->limit);
  assert(0 <= arr->last && arr->last < arr->limit);

  /* data non-NULL check */
  assert(arr->data != NULL);
  return 1;
}

/** @brief Initializes a uba and returns a pointer to it
 *
 *  Fatal errors are only thrown if size == 0 or size == 1 and yet
 *  init_uba() returns NULL (out of heap space)
 *
 *  @param type Element type in the uba
 *  @param limit Actual size of the uba.
 *  @return Pointer to valid uba if successful, NULL if malloc() or
 *          calloc() fails or if limit < 0
 */
uba *uba_new(int limit) {
  if (limit <= 0 ) return NULL;

  /* Convert limit to unsigned */
  uint32_t _limit = (uint32_t) limit;
  assert(0 <= _limit);

  /* Allocate memory for uba struct */
  uba *new_ubap = malloc(sizeof(uba));
  assert(new_ubap != NULL);
  if (new_ubap == NULL) return NULL;

  /* Allocate memory for the array */
  void *data = NULL;
  data = calloc(_limit, sizeof(uint8_t));
  assert(data != NULL);
  if (data == NULL) return NULL;

  /* Set fields of unbounded array uba */
  new_ubap->size = 0;
  new_ubap->limit = _limit;
  new_ubap->first = 0;
  new_ubap->last = 0;
  new_ubap->data = data;

  return new_ubap;
}

/** @brief Resizes the unbounded array
 *
 *  @param arr Pointer to a uba
 *  @param Pointer to the original uba if no resize needed, bigger uba o/w.
 */
uba *uba_resize(uba *arr) {
  assert(is_uba(arr));
  if (arr->size == arr->limit) {

    /* Only resize if sufficient */
    if (arr->size < UINT32_MAX / 2) {
      uba *new_arr = uba_new(arr->size * 2);
      assert(is_uba(new_arr));

      /* Copy elements over */
      while(arr->size > 0) {
        uint8_t elem = uba_rem(arr);
        uba_add(new_arr, elem);
      }
      /* Free the old array */
      free(arr->data);
      free(arr);

      /* Return new array */
      return new_arr;
    }
    /* at limit but cannot resize, OK for now but error will be thrown
     * if want to add a new element to arr
     */
  }
  return arr;
}

/** @brief Adds new element to the array and resizes if needed
 *
 *  @param arr Unbounded array pointer we want to add to
 *  @param elem Element to add to the unbounded array
 *  @return Void.
 */
void uba_add(uba *arr, uint8_t elem) {
  assert(is_uba(arr));
  assert(arr->size < arr->limit); /* Total memory is 256 MiB and since
                                     each array element is 1 byte we
                                     will never reach max possible limit
                                     of 2^32 */

  /* Insert at index one after last element in array */
  arr->data[arr->last] = elem;

  /* Update last index and increment size*/
  arr->last = (arr->last + 1) % arr->limit;
  arr->size += 1;
  assert(is_uba(arr));

  /* Resize if necessary */
  arr = uba_resize(arr);
  assert(is_uba(arr));
}

/** @brief Removes first character of the uba
 *  @param arr Pointer to uba
 *  @return First character of the uba
 */
uint8_t uba_rem(uba *arr) {
  assert(is_uba(arr));

  /* Get first element and 'remove' from the array, decrement size */
  uint8_t elem = arr->data[arr->first];
  arr->first = (arr->first + 1) % arr->limit;
  arr->size -= 1;
  assert(is_uba(arr));

  /* Return 'popped off' element */
  return elem;
}

/** @brief Checks if uba is empty
 *  @param arr Pointer to uba
 *  @return 1 if empty, 0 otherwise.
 */
int uba_empty(uba *arr) {
  assert(is_uba(arr));
  return arr->size == 0;
}

/** @brief Interrupt handler which reads in raw bytes from keystrokes. Reads
 *         incoming bytes to the keyboard buffer key_buf, which has an
 *         amortized constant time complexity for adding elements. So it
 *         returns quickly
 *
 *  @return Void.
 */
void keybd_int_handler(void) {

  /* Read raw byte and put into raw character buffer */
  uint8_t raw_byte = inb(KEYBOARD_PORT);
  uba_add(key_buf, raw_byte);

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

  /* Intialize the raw_byte buffer */
  key_buf = uba_new(CONSOLE_HEIGHT * CONSOLE_WIDTH);
  assert(key_buf != NULL);
}

/** @brief Keeps calling readchar() until another valid char is read and
 *         returns it.
 *
 *  @return A valid character when readchar() doesn't return -1.
 */
char get_next_char(void) {

    /* Get the next char value off the keyboard buffer */
    int res;
    while((res = readchar()) == -1) continue;
    assert(res >= 0);

    /* Tricky type conversions to avoid undefined behavior */
    char char_value = (uint8_t) (unsigned int) res;
    return char_value;
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

  assert(key_buf != NULL);

  /* uba invariants are checked whenever we access the data structure, and so
   * interrupts are disabled during the entire call in order to prevent
   * changes to the data structure at the start of and at the end of a call.
   */
  disable_interrupts();
  if (uba_empty(key_buf)) {
    enable_interrupts();
    return -1;
  }
  raw_byte next_byte = uba_rem(key_buf);
  enable_interrupts();

  /* Get augmented character */
  aug_char next_char = process_scancode(next_byte);

  /* Get simplified character */
  if (KH_HASDATA(next_char)) {
    if (KH_ISMAKE(next_char)) {
      unsigned char next_char_value = KH_GETCHAR(next_char);
      return (int) (unsigned int) next_char_value;
    }
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
  if (buf == NULL) return -1;

  /* len < 0 so invalid len */
  if (len < 0) return -1;

  /* len == 0 so no need to copy */
  if (len == 0) return 0;

  /* get original cursor position for start of line relative to scroll */
  int start_row, start_col;
  get_cursor(&start_row, &start_col);

  /* Allocate space for temporary buffer */
  char temp_buf[len];

  /* Initialize index into temp_buf */
  int i = 0;
  int written = 0; /* characters written so far */
  char ch;

  while ((ch = get_next_char()) != '\n' && written < len) {

    /* ch, i, written is always in range */
    assert(0 <= i && i < len);
    assert(0 <= written && written < len);

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



