/** @file sleep.c
 *  @brief Sleep interrupt handler and facilities for managing
 *		   sleeping threads
 */
#include <asm.h>				/* outb() */
#include <limits.h>				/* UINT_MAX */
#include <assert.h>				/* affirm */
#include <scheduler.h>			/* get_running_thread(), queue_t */
#include <timer_driver.h>		/* get_total_ticks() */
#include <atomic_utils.h>		/* compare_and_swap_atomic() */
#include <install_handler.h>	/* install_handler_in_idt() */
#include <interrupt_defines.h>	/* INT_CTL_PORT, INT_ACK_CURRENT */
#include <task_manager_internal.h> /* Q MACROS on tcb */
#include <lib_thread_management/mutex.h>	/* mutex_t */

/* Using linked list as a naive priority queue implementation.
 * TODO: Use a heap instead! (or something else, like fibonnaci heaps...) */

/** @brief Whether someone is handling the sleep queue.
 *  Acts as a guard for mux since muxes are not re-entrant
 *  but timer interrupts are. */
static uint32_t handling_sleep_queue;

/** @brief mux for sleep queue and earliest expiry date */
static mutex_t sleep_mux;

/** @brief Earliest date a thread should be woken up, out of all sleeping
 *  threads*/
static unsigned int earliest_expiry_date;

/* Queue for all sleeping threads. */
static queue_t sleep_q;

/* 0 if uninitialized, 1 if initialized, -1 if failed to initialize */
static int sleep_initialized = 0;

static void store_tcb_in_sleep_queue( tcb_t *tcb, void *data );

/** @brief Initializes sleep syscall
 *
 *  @pre Has not been called before.
 *	@return Void. */
static void
init_sleep( void )
{
	affirm(!sleep_initialized);
	Q_INIT_HEAD(&sleep_q);
	mutex_init(&sleep_mux);
	earliest_expiry_date = UINT_MAX;
	handling_sleep_queue = 0;
	sleep_initialized = 1;
}

/** @brief Tick handler for sleep module. Wakes up any threads
 *		   which have slept long enough.
 *
 *	To avoid hurting preemptibility too much, we keep track of
 *	the earliest expiry date and go over the list only if that
 *	date has been reached.
 *
 *  @param total_ticks Number of ticks since startup
 *  @return Void.
 */
void
sleep_on_tick( unsigned int total_ticks )
{
	if (!sleep_initialized)
		init_sleep();

	/* Guard with CAS because locks are not re-entrant */
	if (compare_and_swap_atomic(&handling_sleep_queue, 0, 1) == 1)
		return;

	if (total_ticks < earliest_expiry_date) {
		handling_sleep_queue = 0;
		return;
	}

	/* Reset earliest_expiry_date since we will
	 * remove earliest expired thread(s) */
	earliest_expiry_date = UINT_MAX;

	mutex_lock(&sleep_mux);
	tcb_t *curr = Q_GET_FRONT(&sleep_q);
	tcb_t *next;
	while (curr) {
		/* After curr is removed from the sleep queue and is made runnable, we
		 * can no longer safely operated on its scheduler_queue link. As such,
		 * we must get the next member of the queue before making it runnable.*/
		next = Q_GET_NEXT(curr, scheduler_queue);
		if (curr->sleep_expiry_date <= total_ticks) {
			Q_REMOVE(&sleep_q, curr, scheduler_queue);
			make_thread_runnable(curr);
		} else {
			if (curr->sleep_expiry_date < earliest_expiry_date)
				earliest_expiry_date = curr->sleep_expiry_date;
		}
		curr = next;
	}
	mutex_unlock(&sleep_mux);

	/* Release CAS lock */
	handling_sleep_queue = 0;
}

/** @brief Handler for sleep syscall.
 *
 *  @param ticks Number of ticks to sleep
 *  @return 0 on success, negative value on error */
int
sleep( int ticks )
{
	/* Check for initialization before acknowledging interrupt
	 * to avoid race conditions among multiple sleep calls */
	if (!sleep_initialized)
		init_sleep();

    /* Acknowledge interrupt */
    outb(INT_CTL_PORT, INT_ACK_CURRENT);

	if (ticks < 0)
		return -1;

	if (ticks == 0)
		return 0;

	/* No race condition as only sleep manages sleep_expiry_date */
	tcb_t *me = get_running_thread();
	me->sleep_expiry_date = get_total_ticks() + ticks;

	/* TODO:
	 * We could lock the sleep_q mux here and release it through the callback?
	 * This should ensure no conflicts with the sleep q stuff */
	mutex_lock(&sleep_mux);
	affirm(yield_execution(BLOCKED, NULL, store_tcb_in_sleep_queue, NULL) == 0);

	return 0;
}

/** @brief Callback which stores tcb inside sleep queue.
 *
 *  @param tcb Thread to put in sleep queue.
 *  @param data Ignored.
 *  @return Void.
 *  */
static void
store_tcb_in_sleep_queue( tcb_t *tcb, void *data )
{
	affirm(tcb && tcb->status == BLOCKED);
	if (tcb->sleep_expiry_date < earliest_expiry_date)
		earliest_expiry_date = tcb->sleep_expiry_date;
	/* Since thread not running, might as well use the scheduler queue link! */
	Q_INSERT_TAIL(&sleep_q, tcb, scheduler_queue);
	switch_safe_mutex_unlock(&sleep_mux);
}
