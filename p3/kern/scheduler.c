/** @brief Module for scheduling of threads. */

#include <scheduler.h>
#include <task_manager.h>
#include <stdint.h>
#include <assert.h>
#include <add_one_atomic.h>

/** @brief ID of currently active thread. */
static uint32_t active_thread_tid;

static uint32_t num_active_threads = 0; // FIXME: Hacky. Substitute for queues

static int get_next_runnable_thread ( uint32_t *tid );

/** TODO: Create queues runnable, descheduled and done threads.
 *  There should only be one active thread - so no queue. */

/** TODO: Create data structures to keep track of mutexes. Controlling these from schedule
 *  can make them easier to implement. Furthermore we can have an atomically mutex
 *  by forcing the scheduler to only run the thread in the atomically region. */

uint32_t
get_active_thread_tid ( void )
{
    return active_thread_tid;
}

void
register_new_thread ( uint32_t tid )
{
    /* VERY HACKY. */
    affirm(tid == add_one_atomic(&num_active_threads));
    if (tid == 0)
        active_thread_tid = tid;
}

/** On timer interrupt this function should be called.
 *  TODO: *      - Should this be called each tick and then define its rate?
 *      - Should the timer only call this function at the appropriate rate? */
void
scheduler_on_timer_interrupt( int total_ticks )
{
    /* Only accept interrupt every 2 ms. */
    if (total_ticks % 2) return;

    uint32_t tid;
    if (get_next_runnable_thread(&tid) == 0) {
        uint32_t curr = active_thread_tid;
        active_thread_tid = tid;

        thread_switch(curr, tid);
    }
}

/** Implement round-robin algorithm here */
// FIXME: Not the ideal interface. Perhaps only expose a dequeue_next?
static int
get_next_runnable_thread ( uint32_t *tid )
{
    if (num_active_threads < 2)
        return -1;
    // FIXME: Very hacky but works for now.
    *tid = (active_thread_tid + 1) % num_active_threads;
    return 0;
}
