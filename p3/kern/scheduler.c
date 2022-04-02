/** @file scheduler.c
 *  @brief A round-robin scheduler.
 *
 *  Note: As mutexes are implemented by manipulating the schedulers
 *  so that threads waiting on a lock are not executed, the scheduler
 *  itself uses disable/enable interrupts to protect critical sections.
 *  All critical sections protected this way must be short. */

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
 *  @arg tid			Id of thread to yield to, -1 if any
 *  @return 0 on success, negative value on error */
int
yield_execution( queue_t *store_at, status_t store_status, int tid )
{
	if (!scheduler_init) {
		log_warn("Attempting to call yield but scheduler is not initialized");
		return -1;
	}

	if (tid == get_running_tid())
		return 0; // yielding to self

	tcb_t *tcb;
	if (tid != -1) {
		tcb = find_tcb(tid);
		disable_interrupts(); // No need to disable interrupts for find_tcb
	} else {
		disable_interrupts();
		tcb = Q_GET_FRONT(&runnable_q);
	}

	/* If no one to swap to or can't swap to given tid, just return */
	if (!tcb || tcb->status != RUNNABLE) {
		enable_interrupts();
		return -1;
	}

	/* Add to front of queue and swap to next runnable thread.
	 * run_next_tcb requires interrupts to be disabled when called. */
	Q_INSERT_FRONT(&runnable_q, tcb, thr_queue);
    run_next_tcb(store_at, store_status);
	return 0;
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
    affirm_msg(scheduler_init, "Scheduler uninitialized, cannot add to "
	           "runnable queue!");
	disable_interrupts();
    Q_INSERT_TAIL(&runnable_q, tcb, scheduler_queue);
	enable_interrupts();
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

    tcb_t *tcbp = find_tcb(tid);
    if (!tcbp)
        return -1;

    assert(tcbp->status == UNINITIALIZED);

    /* Add tcb to runnable queue, as any thread starts as runnable */
	disable_interrupts();
    if (first_thread) {
        tcbp->status = RUNNING;
        running_thread = tcbp;
    } else {
        tcbp->status = RUNNABLE;
        Q_INSERT_TAIL(&runnable_q, tcbp, scheduler_queue);
    }
	enable_interrupts();

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
        run_next_tcb(NULL, RUNNABLE);
    }
}

/* ------- HELPER FUNCTIONS -------- */


/** @brief Context switch to next thread, as determined by runnable queue.
 *	PRECONDITION: Interrupts disabled when run_next_tcb called.
 *
 *  @arg store_at Queue in which to store old thread in case store_status
 *				  is blocked. For any other store status scheduler determines
 *				  queue to store thread into.
 *  @arg store_status Status with which to store old thread.
 *  @return Void
 *  */
static void
run_next_tcb( queue_t *store_at, status_t store_status )
{
    if (!scheduler_init) {
        enable_interrupts();
        return;
    }

	/* Pick appropriate queue in which to store currently running thread */
	switch (store_status) {
		case RUNNING:
			panic("Trying to store thread with status RUNNING!");
			break;
		case RUNNABLE:
			affirm(store_at == NULL);
			store_at = &runnable_q;
			break;
		case DESCHEDULED:
			affirm(store_at == NULL);
			store_at = &descheduled_q;
			break;
		case BLOCKED:
			affirm(store_at != NULL);
			break;
		case DEAD:
			affirm(store_at == NULL);
			store_at = &dead_q;
			break;
		case UNINITIALIZED:
			panic("Trying to store thread with status UNINITIALIZED!");
			break;
		default:
			panic("Trying to store thread with unknown status!");
			break;
	}

    tcb_t *to_run;
    /* Do nothing if there's no thread waiting to be run */
    if (!(to_run = Q_GET_FRONT(&runnable_q))) {
        affirm_msg(store_status == RUNNABLE, "DEADLOCK");
        enable_interrupts();
        return;
    }
    Q_REMOVE(&runnable_q, to_run, scheduler_queue);

    assert(to_run->tid != running_thread->tid);

    /* Start context switch */
    tcb_t *running = running_thread;
    assert(running->status == RUNNING);

    to_run->status = RUNNING;
    running_thread = to_run;

    /* Add currently running thread to back of store_at queue */
    running->status = store_status;
    Q_INSERT_TAIL(store_at, running, scheduler_queue);

    /* Let thread know where to come back to on USER->KERN mode switch */
    set_esp0((uint32_t)to_run->kernel_stack_hi);

    context_switch((void **)&(running->kernel_esp),
            to_run->kernel_esp, to_run->owning_task->pd);

    enable_interrupts();
}

