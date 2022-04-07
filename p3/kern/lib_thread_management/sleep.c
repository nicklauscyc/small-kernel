/** @file sleep.c
 *  @brief Sleep interrupt handler and facilities for managing
 *		   sleeping threads
 */
#include <asm.h>				/* outb() */
#include <assert.h>				/* affirm */
#include <scheduler.h>			/* get_running_thread(), queue_t */
#include <timer_driver.h>		/* get_total_ticks() */
#include <install_handler.h>	/* install_handler_in_idt() */
#include <interrupt_defines.h>	/* INT_CTL_PORT, INT_ACK_CURRENT */
#include <lib_thread_management/mutex.h>	/* mutex_t */

/* Using linked list as a naive priority queue implementation.
 * TODO: Use a heap instead! (or something else, like fibonnaci heaps...)
 * */

static mutex_t queue_mux;

static queue_t sleep_q;

/* 0 if uninitialized, 1 if initialized, -1 if failed to initialize */
static int sleep_initialized = 0;

static void store_tcb_in_sleep_queue( tcb_t *tcb, void *data );

static void
init_sleep( void )
{
	if (sleep_initialized)
		return;
	Q_INIT_HEAD(&sleep_q);
	mutex_init(&queue_mux);
	sleep_initialized = 1;
}

/** @brief Tick handler for sleep module. Wakes up any threads
 *		   which have slept long enough.
 *
 * FIXME: If this does too much work we will hurt preemptibility,
 * as this will eat up time of the thread being context switched to.
 * Optionally set a limit on how many threads we wake up each given
 * tick*/
void
sleep_on_tick( unsigned int total_ticks )
{
	if (!sleep_initialized)
		init_sleep();

	mutex_lock(&queue_mux);

	tcb_t *curr = Q_GET_FRONT(&sleep_q);
	while (curr) {
		/* A context_switch here could cause us to try removing
		 * an element from the queue twice */
		disable_interrupts();
		if (curr->sleep_expiry_date <= total_ticks) {
			Q_REMOVE(&sleep_q, curr, scheduler_queue);
			make_thread_runnable(curr->tid);
		}
		curr = Q_GET_NEXT(curr, scheduler_queue);
		enable_interrupts();
	}
	mutex_unlock(&queue_mux);
}

int
sleep( int ticks )
{
	/* Check for initialization before acknowledging interrupt
	 * to avoid race conditions among multiple sleep calls */
	if (!sleep_initialized)
		init_sleep();

    /* Acknowledge interrupt and return */
    outb(INT_CTL_PORT, INT_ACK_CURRENT);

	if (ticks < 0)
		return -1;

	if (ticks == 0)
		return 0;

	/* No race condition as only sleep manages sleep_expiry_date */
	tcb_t *me = get_running_thread();
	me->sleep_expiry_date = get_total_ticks() + ticks;

	affirm(yield_execution(BLOCKED, -1, store_tcb_in_sleep_queue, NULL) == 0);

	return 0;
}

/* Modifications to queue are not guarded by mux since they are done
 * atomically (disable/enable interrupts) inside of yield_execution */
static void
store_tcb_in_sleep_queue( tcb_t *tcb, void *data )
{
	affirm(tcb && tcb->status == BLOCKED);
	/* Since thread not running, might as well use the scheduler queue link! */
	Q_INIT_ELEM(tcb, scheduler_queue);
	Q_INSERT_TAIL(&sleep_q, tcb, scheduler_queue);
}
