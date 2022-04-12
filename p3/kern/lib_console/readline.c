/** @file  readline.c
 *  @brief Readline syscall handler and facilities for managing
 *		   concurrent readline's
 */
#include <asm.h>	/* outb */
#include <assert.h>	/* affirm */
#include <atomic_utils.h> /* compare_and_swap_atomic */

// TODO: Add our readline_char_received_handler or whatever it was called
// to keybd_int_handler

/* Wakeup (aka, make_thread_runnable) first thread and let it process
 * as many characters as it can. As soon as it can no longer process
 * characters, it deschedules itself and puts itself into the front
 * of the readline queue. */

// How about keeping a pointer to first guy? Then just call make_runnable
// on him? Can probably avoid race conditions that way. Issue is if
// first guy has already been serviced and is off doing something else.
// Maybe disable/enable interrupts around sections determining next?


static void add_to_readline_queue( tcb_t *tcb, void *data );
static void mark_curr_blocked( tcb_t *tcb, void *data );
static int readchar( void );
static char get_next_char( void );

/** @brief Queue of threads blocked on readline call. */
static queue_t	readline_queue;

/** @brief Thread being served. Can be blocked, running or runnable. */
static tcb_t   *readline_curr;

/** @brief Whether thread being served is blocked.
 *
 *	Readline_curr may blind write to this, keybd int handler
 *	should atomically compare-and-swap to avoid race conditions. */
static int		curr_blocked;

/** @brief Mutex for readline_curr and readline_queue. */
static mutex_t	readline_mux;

void
init_readline( void )
{
	Q_INIT_HEAD(&readline_queue);
	mutex_init(&readline_mux);
	readline_curr = NULL;
	curr_blocked = 0;
}


int
readline( char *buf, int len )
{
	/* Acquire readline mux. Put ourselves at the back of the queue. */
	mutex_lock(&readline_mux);

	if (readline_curr) {
		// add_to_readline_queue will release the mux
		yield_execution(BLOCKED, -1, add_to_readline_queue, NULL);
	} else {
		readline_curr = get_running_thread(); // aka, me
		mutex_unlock(&readline_mux);
	}

	assert(readline_curr == get_running_thread());
	assert(curr_blocked == 0);

	/* Loop on get_next_char, should be just the usual impl now */
}

/* TODO: Call within keybd interrupt handler */
void
readline_char_arrived_handler( void )
{
	/* If curr blocked, readline_char_arrived_handler
	 * is the only one who can operate on curr blocked and
	 * readline curr. However, we can get another keybd interrupt.
	 *
	 * A simple way to ensure make_runnable is called only once
	 * is a CAS on curr blocked.
	 * */
	if (compare_and_swap_atomic(&curr_blocked, 1, 0)) {
		switch_safe_make_thread_runnable(readline_curr);
	}
}

/* --- HELPERS --- */

static void
add_to_readline_queue( tcb_t *tcb, void *data )
{
	Q_INSERT_TAIL(&readline_q, tcb, scheduler_queue);
	switch_safe_mutex_unlock(&readline_mux);
}


static void
mark_curr_blocked( tcb_t *tcb, void *data )
{
	assert(readline_curr == tcb);
	curr_blocked = 1;
}


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


static char
get_next_char( void )
{
    /* Get the next char value off the keyboard buffer */
    int res;
	/* If no character, deschedule ourselves and wait for user input. */
    while ((res = readchar()) == -1) {
		yield_execution(BLOCKED, -1, mark_curr_blocked, NULL);
	}
    assert(res >= 0);

    /* Tricky type conversions to avoid undefined behavior */
    char char_value = (uint8_t) (unsigned int) res;
    return char_value;
}
