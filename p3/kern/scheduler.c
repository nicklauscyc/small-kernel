/** @file scheduler.c
 *	@brief A round-robin scheduler.
 *
 *	Note: As mutexes are implemented by manipulating the schedulers
 *	so that threads waiting on a lock are not executed, the scheduler
 *	itself uses disable/enable interrupts to protect critical sections.
 *	All critical sections protected this way must be short. */

#include <scheduler.h>
#include <task_manager.h>	/* tcb_t */
#include <task_manager_internal.h> /* To use Q MACROS on tcb */
#include <variable_queue.h> /* Q_NEW_LINK() */
#include <context_switch.h> /* context_switch() */
#include <assert.h>			/* affirm() */
#include <malloc.h>			/* smalloc(), sfree() */
#include <stdint.h>			/* uint32_t */
#include <cr.h>				/* get_esp0() */
#include <logger.h>			/* log_warn() */
#include <asm.h>			/* enable/disable_interrupts() */

#include <simics.h>

/* Timer interrupts every ms, we want to swap every 2 ms. */
#define WAIT_TICKS 2

/* Whether scheduler has been initialized */
static int scheduler_init = 0;

/* Thread queues.*/
static queue_t runnable_q;
static tcb_t *running_thread = NULL; // Currently running thread

static void swap_running_thread( tcb_t *to_run, status_t store_status,
		void (*callback)(tcb_t *, tcb_t *, void *), void *data );
static void switch_threads(tcb_t *running, tcb_t *to_run);

/** @brief Whether the scheduler is initialized
 *
 *	@return 1 if initialized, 0 if not */
int
is_scheduler_init( void )
{
	return scheduler_init;
}

void
print_status(status_t status)
{
	switch (status) {
		case RUNNING:
			log_info("Status is RUNNING");
			break;
		case RUNNABLE:
			log_info("Status is RUNNABLE");
			break;
		case DESCHEDULED:
			log_info("Status is DESCHEDULED");
			break;
		case BLOCKED:
			log_info("Status is BLOCKED");
			break;
		case DEAD:
			log_info("Status is DEAD");
			break;
		case UNINITIALIZED:
			log_info("Status is UNINITIALIZED");
			break;
		default:
			log_info("Status is UNKNOWN");
			break;
	}
}

/** @brief Yield execution of current thread, storing it at
 *		   the runnable queue if store_status is RUNNABLE.
 *
 *	@param store_status Status which currently running thread will take
 *	@param tid			Id of thread to yield to, -1 if any
 *	@param callback		Function to be called atomically with tcb of
 *						current thread. MUST BE SHORT
 *	@return 0 on success, negative value on error */
int
yield_execution( status_t store_status, int tid,
		void (*callback)(tcb_t *, tcb_t *, void *), void *data )
{
	affirm(scheduler_init);
	if (!scheduler_init) {
		log_warn("Attempting to call yield but scheduler is not initialized");
		return -1;
	}

	if (tid == get_running_tid()) {
		affirm_msg(store_status == RUNNABLE, "Trying to yield to self"
				" but with non-RUNNABLE status");
		return 0; // yielding to self
	}

	/* Get tcb to swap to */
	tcb_t *tcb;
	if (tid == -1) {
		disable_interrupts();
		tcb = Q_GET_FRONT(&runnable_q);

		/* If we are coincidentally at the front of the queue,
		 * we only want to yield to ourselves if we want to remain
		 * runnable. */
		if (tcb && tcb->tid == get_running_tid()) {
			/* If store_status is not runnable find someone else. */
			if (store_status != RUNNABLE) {
				Q_REMOVE(&runnable_q, tcb, scheduler_queue);
				tcb = Q_GET_FRONT(&runnable_q);
			}
		}

		if (!tcb) {
			if (store_status == RUNNABLE) {
				enable_interrupts();
				return 0; /* Yield to self*/
			} else {
				print_status(store_status);
				panic("DEADLOCK, scheduler has no one to run!");
			}
		}

		/* Any thread in runnable queue is either running (and also
		 * in runnable queue because of a yield), or is just runnable. */
		assert(get_tcb_status(tcb) == RUNNABLE ||
				get_tcb_status(tcb) == RUNNING);

	} else {
		/* find_tcb() is already guarded by a mutex */
		tcb = find_tcb(tid); // FIXME: Could this cause an issue? Recursion or smth
		if (!tcb) {
			log_warn("Trying to yield_execution to non-existent"
					 " thread with tid %d", tid);
			return -1; /* Thread not found */
		}
		disable_interrupts();
		if (get_tcb_status(tcb) != RUNNABLE) {
			log_warn("Trying to yield_execution to non-runnable"
					 " thread with tid %d", tid);
			enable_interrupts();
			return -1;
		}
	}

	/* If this thread is to be made not-runnable, ensure it is not
	 * on the runnable queue. The currently running thread can be
	 * on the runnable queue if it was yielded to previously. */
	if (store_status != RUNNABLE &&
			Q_IN_SOME_QUEUE(running_thread, scheduler_queue)) {
		/* tcb must be in scheduler_q, therefore we can safely remove it */
		Q_REMOVE(&runnable_q, running_thread, scheduler_queue);
	}

	swap_running_thread(tcb, store_status, callback, data);
	return 0;
}

/** @brief Gets tid of currently active thread.
 *
 *	@return Id of running thread */
int
get_running_tid( void )
{
	/* If running_thread is NULL, we have a single thread with tid 0. */
	if (!running_thread) {
		return 0;
	}
	return running_thread->tid;
}

/** @brief Gets currently active thread.
 *
 *	@return Running thread, NULL if no such thread */
tcb_t *
get_running_thread( void )
{
	return running_thread;
}

/** @brief Gets pointer to PCB that currently running thread belongs to
 *
 *  @return non-NULL pointer of owning task of currently running thread if
 *          currently running thread is non-NULL, NULL otherwise
 */
pcb_t *
get_running_task( void )
{
	if (!running_thread) {
		affirm(!scheduler_init);
		return NULL;
	} else {
		affirm(running_thread->owning_task);
		return running_thread->owning_task;
	}
}


/** @brief Initializes scheduler and registers its first thread.
 *
 *	@return 0 on success, -1 on error
 */
static int
init_scheduler( void )
{
	/* Initialize once and only once */
	affirm(!scheduler_init);

	Q_INIT_HEAD(&runnable_q);

	scheduler_init = 1;

	return 0;
}

static int
make_thread_runnable_helper( uint32_t tid, int switch_safe )
{
	log("Making thread %d runnable", tid);

	tcb_t *tcbp = find_tcb(tid);
	if (!tcbp)
		return -1;

	/* Add tcb to runnable queue, as any thread starts as runnable */
	disable_interrupts();
	if (tcbp->status == RUNNABLE || tcbp->status == RUNNING) {
		log_warn("Trying to make runnable thread %d runnable again", tid);
		if (!switch_safe)
			enable_interrupts();
		return -1;
	}

	if (!scheduler_init) {
		init_scheduler();
		tcbp->status = RUNNING;
		running_thread = tcbp;
	} else {
		if (tcbp->status == UNINITIALIZED || switch_safe) {
			tcbp->status = RUNNABLE;
			Q_INSERT_TAIL(&runnable_q, tcbp, scheduler_queue);
		} else {
			/* "Improve" preemptibility by immediately swapping to thread
			 * being made runnable. To avoid doing so for newly registered
			 * threads, only swap immediately if status != UNINITIALIZED. */
			swap_running_thread(tcbp, RUNNABLE, NULL, NULL);
		}
	}

	if (!switch_safe)
		enable_interrupts();

	return 0;
}

int
switch_safe_make_thread_runnable( uint32_t tid )
{
	return make_thread_runnable_helper(tid, 1);
}

/** @brief Registers thread with scheduler. After this call,
 *		   the thread may be executed by the scheduler.
 *
 *	@param tid Id of thread to register
 *
 *	@return 0 on success, negative value on error */
/* TODO: Think of synchronization here*/
int
make_thread_runnable( uint32_t tid )
{
	return make_thread_runnable_helper(tid, 0);
}

void
scheduler_on_tick( unsigned int num_ticks )
{
	if (!scheduler_init)
		return;

	if (num_ticks % WAIT_TICKS == 0) {
		disable_interrupts();

		tcb_t *to_run;
		/* Do nothing if there's no thread waiting to be run */
		if (!(to_run = Q_GET_FRONT(&runnable_q))) {
			enable_interrupts();
			return;
		}
		Q_REMOVE(&runnable_q, to_run, scheduler_queue);

		swap_running_thread(to_run, RUNNABLE, NULL, NULL);
	}
}

/* ------- HELPER FUNCTIONS -------- */

/** @brief Swaps the running thread to to_run.
 *
 *	@pre Interrupts disabled when called.
 *
 *	@param to_run Thread to run next
 *	@param store_at	Queue in which to store old thread in case store_status
 *					is blocked. For any other store status scheduler determines
 *					queue to store thread into.
 *	@param store_status	Status with which to store old thread.
 *	@return Void.
 */
static void
swap_running_thread( tcb_t *to_run, status_t store_status,
		void (*callback)(tcb_t *, tcb_t *, void *), void *data )
{
	assert(to_run);
	affirm_msg(scheduler_init, "Scheduler has to be initialized before calling "
			   "swap_running_thread");

	/* yield_execution will not remove the thread it yields to from
	 * the runnable queue. Therefore, we give it more CPU cycles without
	 * an explicit context switch by just returning.*/

	/* No-op if we swap with ourselves */
	if (to_run->tid == running_thread->tid) {
		affirm(store_status == RUNNABLE);
		enable_interrupts();
		return;
	}

	/* Validate store status */
	switch (store_status) {
		case RUNNING:
			panic("Trying to store thread with status RUNNING!");
			break;
		case RUNNABLE:
			break;
		case DESCHEDULED:
			break;
		case BLOCKED:
			break;
		case DEAD:
			break;
		case UNINITIALIZED:
			panic("Trying to store thread with status UNINITIALIZED!");
			break;
		default:
			panic("Trying to store thread with unknown status!");
			break;
	}

	assert(running_thread->status == RUNNING);

	tcb_t *running = running_thread;
	running->status = store_status;

	/* Data structure for other statuses are managed by their own components,
	 * scheduler is only responsible for managing runnable/running threads. */
	/* If running thread is already in runnable queue, don't insert again */
	if (store_status == RUNNABLE && !Q_IN_SOME_QUEUE(running, scheduler_queue))
		Q_INSERT_TAIL(&runnable_q, running, scheduler_queue);
	else if (callback) {
		/* FIXME: What happens if the callback calls a yield_execution? */
		callback(running, to_run, data);
	}

	/* FIXME when a thread calls _vanish() on itself, it needs to remain
	 * RUNNING until it is done free-ing itself, which is called via
	 * the callback set to free_tcb_callback, so we update running->status
	 * to DEAD only after free_tcb_callback() has completed */


	/* Update running thread after, since callback expects to be called by
	 * original running thread. */
	to_run->status = RUNNING;
	running_thread = to_run;

	/* Interrupts are enabled inside context switch, once it's safe to do so. */
	switch_threads(running, to_run);

	/* Nothing which must run should be placed after switch_threads as,
	 * when we context switch to a new thread for the first time, the stack
	 * will be setup for it to return directly to the call_fork asm wrapper. */
}

static void
switch_threads(tcb_t *running, tcb_t *to_run)
{
	assert(running && to_run);
	assert(to_run->tid != running->tid);

	/* Let thread know where to come back to on USER->KERN mode switch */
	set_esp0((uint32_t)to_run->kernel_stack_hi);

	context_switch((void **)&(running->kernel_esp), to_run->kernel_esp);
}


