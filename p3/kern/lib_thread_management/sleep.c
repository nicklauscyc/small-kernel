/** @file sleep.c
 *  @brief Contains sleep interrupt handler
 */
#include <scheduler.h>			/* get_running_tid() */
#include <asm.h>				/* outb() */
#include <install_handler.h>	/* install_handler_in_idt() */
#include <interrupt_defines.h>	/* INT_CTL_PORT, INT_ACK_CURRENT */



/* Using linked list as a naive priority queue implementation.
 * TODO: Use a heap instead! (or something else, like fibonnaci heaps...)
 * */

static mutex_t queue_mux;

static queue_t sleep_q;

/* 0 if uninitialized, 1 if initialized, -1 if failed to initialize */
static int sleep_initialized = 0;

static void add_to_sleeping_stack( int ticks );


/* TODO: Call from the timer_driver tick function */
void
sleep_on_tick( int total_ticks )
{
	mutex_lock(&stack_mux);

	// TODO: Go over all members of sleep_q and check for expiry date
	// call make_thread_runnable on all those threads

	mutex_unlock(&stack_mux);
}

void
init_sleep( void )
{
	Q_INIT_HEAD(&sleep_q);
	mutex_init(&stack_mux);
	sleep_initialized = 1;
}

int
sleep( int ticks )
{
    /* Acknowledge interrupt and return */
    outb(INT_CTL_PORT, INT_ACK_CURRENT);

	if (ticks < 0)
		return -1;

	/* deschedule, then every timer interrupt check if num_ticks has been reached? */
	add_to_sleeping_stack();

	/* TODO: Do the return values match? */
	return yield_execution(NULL, BLOCKED, -1);
}

static void
add_to_sleeping_stack( int ticks )
{
	mutex_lock(&stack_mux);

	// TODO: init elem
	// TODO: Add to queue using scheduler_queue link

	mutex_unlock(&stack_mux);
}
