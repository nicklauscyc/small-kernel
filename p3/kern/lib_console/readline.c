/** @file  readline.c
 *	@brief Readline syscall handler and facilities for managing
 *		   concurrent readline's
 */
#include <asm.h>			/* outb */
#include <ctype.h>			/* isprint() */
#include <assert.h>			/* affirm */
#include <string.h> 		/* memcpy */
#include <stdint.h>			/* uint32_t */
#include <stddef.h>			/* NULL */
#include <malloc.h>			/* smalloc/sfree */
#include <console.h>		/* putbyte() */
#include <keyhelp.h>		/* process_scancode() */
#include <scheduler.h>		/* status_t, queue_t, make_runnable  */
#include <keybd_driver.h>	/* aug_char*/
#include <atomic_utils.h>	/* compare_and_swap_atomic */
#include <keybd_driver.h>	/* get_next_aug_char */
#include <task_manager.h>	/* tcb_t */
#include <video_defines.h>	/* CONSOLE_HEIGHT, CONSOLE_WIDTH */
#include <variable_queue.h> /* Q_ macros */
#include <memory_manager.h> /* READ_WRITE, is_valid_user_pointer */
#include <task_manager_internal.h> /* struct tcb */
#include <lib_thread_management/mutex.h> /* mutex_t */
#include <logger.h>

static void mark_curr_blocked( tcb_t *tcb, void *data );
static char get_next_char( void );
static int readchar( void );
static int _readline(char *buf, int len);

/** @brief Thread being served. Can be blocked, running or runnable. */
static tcb_t   *readline_curr;

/** @brief Whether thread being served is blocked.
 *
 *	Readline_curr may blind write to this, keybd int handler
 *	should atomically compare-and-swap to avoid race conditions. */
static uint32_t	curr_blocked;

/** @brief Mutex for readline_curr and readline_q. */
static mutex_t	readline_mux;


/** @brief Initialize readline.
 *
 *  @return Void.
 *  */
void
init_readline( void )
{
	mutex_init(&readline_mux);
	readline_curr = NULL;
	curr_blocked = 0;
}


/** @brief Readline syscall handler
 *
 *  @param buf Buffer in which to write characters
 *  @param len Max number of characters to write in buf
 *  @return Number of bytes read on success, negative value on error */
int
readline( int len, char *buf )
{
	if (len < 0) return -1;
	if (len == 0) return 0;
	if (len > CONSOLE_WIDTH * CONSOLE_HEIGHT) return -1;
	for (int i=0; i < len; ++i) {
		if (!is_valid_user_pointer(buf + i, READ_WRITE))
			return -1;
	}

	/* Acquire readline mux. Put ourselves at the back of the queue. */
	mutex_lock(&readline_mux);

	readline_curr = get_running_thread();

	int res = _readline(buf, len);

	/* Done. Check if anyone else waiting in line */
	mutex_unlock(&readline_mux);

	return res;
}

/** @brief Readline helper.
 *
 *	Does not guarantee mutual exclusion of reads but deschedules and waits
 *	for chars to arrive while processing.
 *
 *  @param buf Buffer in which to write characters
 *  @param len Max number of characters to write in buf
 *  @return Number of bytes read on success, negative value on error */

static int
_readline(char *buf, int len)
{
  	int start_row, start_col;
  	get_cursor(&start_row, &start_col);

	assert(len <= CONSOLE_WIDTH * CONSOLE_HEIGHT);
	/* Too large to comfortably fit on kernel stack */
	char *temp_buf;
	temp_buf = smalloc(CONSOLE_WIDTH * CONSOLE_HEIGHT);

  	int i = 0;
  	int written = 0; /* characters written so far */
  	char ch;

  	while ((ch = get_next_char()) != '\n' && written < len) {

		/* ch, i, written is always in range */
  	  	assert(0 <= i && i < len);
  	  	assert(0 <= written && written < len);

  	  	/* If at front of buffer, Delete the character if backspace */
  	  	if (ch == '\b') {

			/* If at start_row/col, do nothing so as to not delete prompt */
  	  	  	int row, col;
  	  	  	get_cursor(&row, &col);

  	  	  	if (!(row * CONSOLE_WIDTH + col >= start_row * CONSOLE_WIDTH + start_col)) {
				continue;
			}

  	  	  	if (i > 0) {
				assert((row != start_row || col != start_col));

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
			temp_buf[i] = ch;
			i++;
			if (i > written) written = i;
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

	sfree(temp_buf, CONSOLE_HEIGHT * CONSOLE_WIDTH);

  	return written;
}

/** @brief Call to let readline know new characters have arrived.
 *
 *	NOTE: This is called within the keybd interrupt handler, after the
 *	signal is acknowledged.
 *
 *	@return Void.
 *	*/
void
readline_char_arrived_handler( void )
{
	/* If curr_blocked, readline_char_arrived_handler is the only one who can
	 * operate on curr_blocked and readline_curr. However, we can get another
	 * keybd interrupt, leading to a race condition. A simple way to ensure
	 * make_runnable is called only once is a CAS on curr_blocked. */
	int curr_blocked_start = curr_blocked;

	if (compare_and_swap_atomic(&curr_blocked, 1, 0)) {
		switch_safe_make_thread_runnable(readline_curr);
		log("readline_char_arrived_handler(): "
				 "curr_blocked_start:%d "
		         "curr_block:%d "
		         "readline_curr:%p, "
		         "readline_curr->tid:%d ",
		         curr_blocked_start, curr_blocked, readline_curr,
				 readline_curr->tid);
	}
}

/* --- HELPERS --- */

/** @brief Callback to mark tcb as blocked.
 *
 *  @param tcb Tcb to mark blocked
 *  @param data Unused
 *  @return Void.
 *  */
static void
mark_curr_blocked( tcb_t *tcb, void *data )
{
	assert(readline_curr == tcb);
	curr_blocked = 1;
}

/** @brief Gets next char from user input,
 *		   blocking if no more characters to read
 *
 *  @return Next character from user input
 *  */
static char
get_next_char( void )
{
	/* Get the next char value off the keyboard buffer */
	int res;
	/* If no character, deschedule ourselves and wait for user input. */
	while ((res = readchar()) == -1) {
		log("mark self as blocked, running_thread:%p",
				get_running_thread());
		yield_execution(BLOCKED, NULL, mark_curr_blocked, NULL);
	}
	assert(res >= 0);

	/* Tricky type conversions to avoid undefined behavior */
	char char_value = (uint8_t) (unsigned int) res;
	return char_value;
}

/** @brief Tries to read a character from user input.
 *
 *  @return Character on success, negative value on error */
static int
readchar( void )
{
	aug_char next_char;
	while (get_next_aug_char(&next_char) == 0) {
		if (KH_HASDATA(next_char) && KH_ISMAKE(next_char)) {
			unsigned char next_char_value = KH_GETCHAR(next_char);
            return (int) (unsigned int) next_char_value;
		}
	}
	return -1;
}
