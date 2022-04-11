/** @file  readline.c
 *  @brief Readline syscall handler and facilities for managing
 *		   concurrent readline's
 */
#include <asm.h>	/* outb */
#include <assert.h>	/* affirm */


// TODO: readline_char_arrived_handler() inside keybd_int_handler
//void
//init_keybd( void )
//{
//	init_buf(&key_buf, uint8_t, CONSOLE_WIDTH * CONSOLE_HEIGHT);
//	is_buf(&key_buf);
//	return;
//}
//
//void
//keybd_int_handler( void )
//{
//
//	/* Read raw byte and put into raw character buffer */
//  	uint8_t raw_byte = inb(KEYBOARD_PORT);
//  	buf_insert(&key_buf, raw_byte);
//
//  	/* Acknowledge interrupt */
//  	outb(INT_CTL_PORT, INT_ACK_CURRENT);
//		// FIXME: Any race conditions here? What if two people run this
//		for the same one character? What happens? How can we prevent it?
//  	/* Let readline know it has received a new character */
//		readline_char_arrived_handler();
//}

static queue_t readline_queue;
static mutex_t readline_mux;

void
init_readline( void )
{
	Q_INIT_HEAD(&readline_queue);
	mutex_init(&readline_mux);
}

int
readline( char *buf, int len )
{

	/* If no one in queue, and line of input available, service request.
	 * Otherwise place thread in readline_wait queue. */

	/* Whenever the user types \n we know that we can wake someone up
	 * to service their request. Whatever bytes are not consumed will
	 * be given to the next reader whenever the user types \n again. */


}

int
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

static void
store_at_front_read_queue( tcb_t *tcb, void *data )
{
	Q_INSERT_FRONT(&readline_q, tcb, scheduler_queue);
	switch_safe_mutex_unlock(&readline_mux);
}

char get_next_char(void) {

    /* Get the next char value off the keyboard buffer */
    int res;
	/* If no character, deschedule ourselves and wait for user input. */
    while ((res = readchar()) == -1) {
		// Mutex required for store_at_front_read_queue
		mutex_lock(&readline_mux);
		yield_execution(BLOCKED, -1, store_at_front_read_queue, NULL);
	}
    assert(res >= 0);

    /* Tricky type conversions to avoid undefined behavior */
    char char_value = (uint8_t) (unsigned int) res;
    return char_value;
}

int
readline_char_arrived_handler( void )
{
	/* Wakeup (aka, make_thread_runnable) first thread and let it process
	 * as many characters as it can. As soon as it can no longer process
	 * characters, it deschedules itself and puts itself into the front
	 * of the readline queue. */

	// TODO: Can we really acquire the lock here? Might be bad, as thread
	// running interrupt will be made non-runnable

	// How about keep pointer to first guy? Then just call make_runnable
	// on him? Can probably avoid race conditions that way.
}
