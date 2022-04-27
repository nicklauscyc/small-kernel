/** @file keybd_driver.c
 *	@brief Contains functions that help the user type into the console
 *
 *	@bug No known bugs.
 *
 *
 *	Since unbounded arrays are used, let 'size' be the number of elements in the
 *	array that we care about, and 'limit' be the actual length of the array.
 *	Whenever the size of the array == its limit, the array limit is doubled.
 *	Doubling is only allowed if the current limit <= UINT32_MAX to prevent
 *	overflow.
 *
 *	Indexing into circular arrays is just done modulo the limit of the array.
 *
 *	The two functions in the keyboard driver interface readchar() amd readline()
 *	are closely connected to one another. The specification for readline()
 *	states that "Characters not placed into the specified buffer should remain
 *	available for other calls to readline() and/or readchar()." To put it
 *	concisely, we have the implication:
 *
 *	  char not committed to any buffer => char available for other calls
 *
 *	  i.e.
 *
 *	  char not available for other calls => char committed to some buffer
 *
 *	The question now is under what circumstance do we promise that a char
 *	is not available? It is reasonable to conclude that the above implication
 *	is in fact a bi-implication. Therefore:
 *
 *	  char not available for other calls <=> char committed to some buffer
 *
 *	Now since readchar() always reads the next character in the keyboard buffer,
 *	it is then conceivable that if a readchar() not called by readline() takes
 *	the next character off the keyboard buffer, then readline() would "skip"
 *	a character. But then, the specification says that:
 *
 *	"Since we are operating in a single-threaded environment, only one of
 *	readline() or readchar() can be executing at any given point."
 *
 *	Therefore we will never have readline() and readchar() concurrently
 *	executing in seperate threads in the context of the same process, since
 *	when we speak of threads, we refer to threads in the same process.
 *
 *	Therefore we need not worry about the case where a readchar() that is
 *	not invoked by readline() is called in the middle of another call to
 *	readline().
 *
 *	@author Nicklaus Choo (nchoo)
 *	@bug No known bugs
 */

#include <keybd_driver.h>

#include <asm.h>					/* process_scancode() */
#include <ctype.h>					/* isprint() */
#include <malloc.h>					/* calloc */
#include <stddef.h>					/* NULL */
#include <assert.h>					/* assert() */
#include <string.h>					/* memcpy() */
#include <console.h>				/* putbyte() */
#include <keyhelp.h>				/* process_scancode() */
#include <video_defines.h>			/* CONSOLE_HEIGHT, CONSOLE_WIDTH */
#include <variable_buffer.h>		/* generic buffer macros */
#include <interrupt_defines.h>		/* INT_CTL_PORT */
#include <lib_console/readline.h>	/* readline_char_arrived_handler() */

/* Keyboard buffer */
new_buf(keyboard_buffer_t, uint8_t, CONSOLE_WIDTH * CONSOLE_HEIGHT);
static keyboard_buffer_t key_buf;

int readchar(void);

#include <logger.h>

static int keys = 0;
/** @brief Interrupt handler which reads in raw bytes from keystrokes. Reads
 *		   incoming bytes to the keyboard buffer key_buf, which has an
 *		   amortized constant time complexity for adding elements. So it
 *		   returns quickly
 *
 *	@return Void.
 */
void keybd_int_handler(void)
{
	/* Read raw byte and put into raw character buffer */
	uint8_t raw_byte = inb(KEYBOARD_PORT);
	buf_insert(&key_buf, raw_byte);

	keys++;
	log("keybd_int_handler(): received:%d", keys);
	/* Acknowledge interrupt */
	outb(INT_CTL_PORT, INT_ACK_CURRENT);
	log("keybd_int_handler(): ack:%d", keys);

	/* Let readline module know new characters have arrived. */
	readline_char_arrived_handler();
	log("keybd_int_handler(): executed readline_char_arrived()");

}

/** @brief Initialize the keyboard interrupt handler and associated data
 *		   structures
 *
 *	Memory for keybd_buf is allocated here.
 *
 *	@return Void.
 */
void
init_keybd( void )
{
	init_buf(&key_buf, uint8_t, CONSOLE_WIDTH * CONSOLE_HEIGHT);
	is_buf(&key_buf);
	return;
}

/*********************************************************************/
/*																	 */
/* Keyboard driver interface										 */
/*																	 */
/*********************************************************************/

/** @brief Gets next aug char.
 *
 *  Not thread-safe. */
int
get_next_aug_char( aug_char *next_char )
{
	if (buf_empty(&key_buf)) {
		enable_interrupts();
		return -1;
	}
	uint8_t next_byte;
	buf_remove(&key_buf, &next_byte);
	is_buf(&key_buf);

	*next_char = process_scancode(next_byte);
	return 0;
}
