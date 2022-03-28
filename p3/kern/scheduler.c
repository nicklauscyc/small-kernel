/** @file scheduler.c
 *  @brief A round-robin scheduler. */

#include <scheduler.h>
#include <task_manager.h>   /* tcb_t */
#include <variable_queue.h> /* Q_NEW_LINK() */
#include <context_switch.h> /* context_switch() */
#include <assert.h>         /* affirm() */
#include <malloc.h>         /* smalloc(), sfree() */
#include <stdint.h>         /* uint32_t */
#include <cr.h>             /* get_esp0() */
#include <logger.h>         /* log_warn() */
#include <asm.h>            /* enable/disable_interrupts() */

/* Timer interrupts every ms, we want to swap every 2 ms. */
#define WAIT_TICKS 2

/* Whether scheduler has been initialized */
static int scheduler_init = 0;

/* Thread queues.*/
static queue_t runnable_q;
static queue_t descheduled_q;
static queue_t dead_q;
static tcb_t *running_thread = NULL; // Currently running thread

static void run_next_tcb( queue_t *store_at, status_t store_status );

/** @brief Whether the scheduler is initialized
 *
 *  @return 1 if initialized, 0 if not */
int
is_scheduler_init( void )
{
	return scheduler_init;
}

/** @brief Yield execution of current thread, storing it at
 *         the designated queue.
 *
 *  @arg store_at       Queue in which to store thread
 *  @arg store_status   Status with which to store thread
 *
 *  @return Void. */
void
yield_execution( queue_t *store_at, status_t store_status )
{
    /* Do not want others to be able to directly call run_next_tcb.
     * TODO: Should any checks go here? */

    run_next_tcb(store_at, store_status);
}

// FIXME: Maybe crash if !scheduler_init???
/** @brief Gets tid of currently active thread.
 *
 *  @return Id of running thread on success, negative value on error */
int
get_running_tid( void )
{
    if (!running_thread)
        return -1;
    return running_thread->tid;
}

void
add_to_runnable_queue( tcb_t *tcb )
{
    if (!scheduler_init)
        log_warn("Trying to add to runnable queue with uninitialized \
                scheduler. ");
    Q_INSERT_TAIL(&runnable_q, tcb, thr_queue);
}

/** @brief Initializes scheduler and registers its first thread.
 *
 *  Must be called once and only once. A thread must be supplied
 *  as the scheduler maintains the invariant that there is always
 *  one thread running. If this cannot be met, we are facing a deadlock.
 *
 *  @arg tid Id of thread to start running.
 *  @return 0 on success, -1 on error
 */
int
init_scheduler( uint32_t tid )
{
	/* Initialize once and only once */
	affirm(!scheduler_init);

	Q_INIT_HEAD(&runnable_q);
	Q_INIT_HEAD(&descheduled_q);
	Q_INIT_HEAD(&dead_q);

	scheduler_init = 1;

	return 0;
}

/** @brief Registers thread with scheduler. After this call,
 *         the thread may be executed by the scheduler.
 *
 *  @arg tid Id of thread to register
 *
 *  @return 0 on success, negative value on error */
/* TODO: Think of synchronization here*/
int
register_thread(uint32_t tid)
{
    int first_thread = !scheduler_init;
    if (!scheduler_init) {
        init_scheduler(tid);
    }

    tcb_t *tcb = find_tcb(tid);
    if (!tcb)
        return -1;

    assert(tcb->status == UNINITIALIZED);

    /* Add tcb to runnable queue, as any thread starts as runnable */
    if (first_thread) {
        tcb->status = RUNNING;
        running_thread = tcb;
    } else {
        tcb->status = RUNNABLE;
        Q_INSERT_TAIL(&runnable_q, tcb, thr_queue);
    }

    return 0;
}

void
scheduler_on_tick( unsigned int num_ticks )
{
    if (!scheduler_init)
        return;
    if (num_ticks % WAIT_TICKS == 0) /* Context switch every 2 ms */
    {
        disable_interrupts();
        run_next_tcb(&runnable_q, RUNNABLE);
    }
}

/* ------- HELPER FUNCTIONS -------- */


/** PRECONDITION: Interrupts disabled when run_next_tcb called.
 *  @arg store_at Queue in which to store old thread. */
static void
run_next_tcb( queue_t *store_at, status_t store_status )
{
    if (!scheduler_init) {
        enable_interrupts();
        return;
    }

    tcb_t *to_run;
    /* Do nothing if there's no thread waiting to be run */
    if (!(to_run = Q_GET_FRONT(&runnable_q))) {
        affirm_msg(store_status == RUNNABLE, "DEADLOCK");
        enable_interrupts();
        return;
    }
    Q_REMOVE(&runnable_q, to_run, thr_queue);

    assert(to_run->tid != running_thread->tid);

    /* Start context switch */
    tcb_t *running = running_thread;
    assert(running->status == RUNNING);

    to_run->status = RUNNING;
    running_thread = to_run;

    /* Add currently running thread to back of store_at queue */
    running->status = store_status;
    Q_INSERT_TAIL(store_at, running, thr_queue);

    /* Let thread know where to come back to on USER->KERN mode switch */
    set_esp0((uint32_t)to_run->kernel_stack_hi);

    context_switch((void **)&(running->kernel_esp),
            to_run->kernel_esp, to_run->owning_task->pd);

    enable_interrupts();
}

