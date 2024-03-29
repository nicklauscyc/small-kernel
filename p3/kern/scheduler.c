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
#include <iret_travel.h>
#include <simics.h>

/* Timer interrupts every ms, we want to swap every 2 ms. */
#define WAIT_TICKS 2

/* Whether scheduler has been initialized */
static int scheduler_init = 0;

/* Whether currently in multi-threaded environment */
static int multi_threads = 0;

/* Thread queues.*/
static queue_t runnable_q;
static tcb_t *running_thread = NULL; // Currently running thread

static void swap_running_thread( tcb_t *to_run );

static void switch_threads(tcb_t *running, tcb_t *to_run);

/** @brief Whether the scheduler is initialized
 *
 *	@return 1 if initialized, 0 if not. */
int
is_scheduler_init( void )
{
	return scheduler_init;
}

/** @brief Gets status as a string.
 *
 *  @param status Status to fetch string for
 *  @return Status as a string */
char *
status_str(status_t status)
{
	switch (status) {
		case RUNNING:
			return "RUNNING";
			break;
		case RUNNABLE:
			return "RUNNABLE";
			break;
		case DESCHEDULED:
			return "DESCHEDULED";
			break;
		case BLOCKED:
			return "BLOCKED";
			break;
		case DEAD:
			return "DEAD";
			break;
		case UNINITIALIZED:
			return "UNINITIALIZED";
			break;
		default:
			return "UNKNOWN";
			break;
	}
}

/** @brief Get the next tcb that should be ran while
 *		   managing the runnable queue.
 *
 *  @return A runnable tcb not in the runnable queue */
tcb_t *
get_next_run( void )
{
	tcb_t *tcb = Q_GET_FRONT(&runnable_q);
	if (!tcb) panic("DEADLOCK");
	Q_REMOVE(&runnable_q, tcb, scheduler_queue);

	assert(get_tcb_status(tcb) == RUNNABLE);
	return tcb;
}

/** @brief Add thread to the runnable queue.
 *
 *  @param tcb Thread to add to the runnable queue
 *  @return Void. */
void
add_to_run( tcb_t *tcb )
{
	tcb->status = RUNNABLE;
	Q_INSERT_TAIL(&runnable_q, tcb, scheduler_queue);
}

/** @brief Yield execution of current thread, storing it at
 *		   the runnable queue if store_status is RUNNABLE.
 *
 *  @pre scheduler must be initialized
 *	@param store_status Status which currently running thread will take
 *	@param tcb			Thread to yield to, NULL if any
 *	@param callback		Function to be called atomically with tcb of
 *						current thread. MUST BE SHORT
 *	@return 0 on success, negative value on error */
int
yield_execution( status_t store_status, tcb_t *tcb,
		void (*callback)(tcb_t *, void *), void *data )
{
	if (!scheduler_init) {
		panic("Attempting to call yield but scheduler is not initialized");
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
	disable_interrupts();

	if (tcb && (get_tcb_status(tcb) != RUNNABLE)
		&& (get_tcb_status(tcb) != RUNNING)) {
		log_warn("Trying to yield_execution to non-runnable or running"
				 " thread with tid %d", tcb->tid);
		enable_interrupts();
		return -1;
	}

	/* Callback/Add self to end of queue */
	running_thread->status = store_status;
	if (store_status == RUNNABLE)
		add_to_run(running_thread);
	else if (callback)
		callback(running_thread, data);

	/* Get tcb to swap to */
	if (!tcb)
		tcb = get_next_run();

	else {
		/* Ensure this thread is no longer in the runnable queue */
		/* In the case where a waiting thread is made runnable by a vanished
		 * child task thread that wakes it up, the waiting thread's TCB
		 * will not be in the runnable_q and so no removing is needed */
		if (Q_IN_SOME_QUEUE(tcb, scheduler_queue))
			Q_REMOVE(&runnable_q, tcb, scheduler_queue);
	}

	swap_running_thread(tcb);
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

/** @brief Registers thread with scheduler. After this call,
 *		   the thread may be executed by the scheduler.
 *
 *	@param tid Id of thread to register
 *	@param switch_safe Whether to avoid context switching
 *	@return 0 on success, negative value on error */
static int
make_thread_runnable_helper( tcb_t *tcbp, int switch_safe )
{
	if (!tcbp)
		return -1;

	/* execute user program calls this with init */
	if (!scheduler_init) {
		init_scheduler();
	}

	log("Making thread %d runnable", tcbp->tid);


	/* Add tcb to runnable queue, as any thread starts as runnable */
	disable_interrupts();

	if (tcbp->status == RUNNABLE || tcbp->status == RUNNING) {
		log_warn("Trying to make runnable thread %d runnable again", tcbp->tid);
		if (!switch_safe)
			enable_interrupts();
		return -1;
	}

	/* tcbp->status == UNINITIALIZED when first created in
	 * execute_user_program */
	if (tcbp->status == UNINITIALIZED || switch_safe) {
		add_to_run(tcbp);
	} else {
		/* "Improve" preemptibility by immediately swapping to thread
		 * being made runnable. To avoid doing so for newly registered
		 * threads, only swap immediately if status != UNINITIALIZED. */
		add_to_run(running_thread);
		swap_running_thread(tcbp);
	}

	if (!switch_safe)
		enable_interrupts();

	return 0;
}


/** @brief Registers thread with scheduler. After this call,
 *		   the thread may be executed by the scheduler. This
 *		   call will NOT trigger a context switch, nor will it
 *		   enable interrupts.
 *
 *	@param tid Id of thread to register
 *	@return 0 on success, negative value on error */
int
switch_safe_make_thread_runnable( tcb_t *tcbp )
{
	return make_thread_runnable_helper(tcbp, 1);
}

/** @brief Registers thread with scheduler. After this call,
 *		   the thread may be executed by the scheduler.
 *
 *	@param tid Id of thread to register
 *
 *	@return 0 on success, negative value on error */
int
make_thread_runnable( tcb_t *tcbp )
{
	return make_thread_runnable_helper(tcbp, 0);
}

/** @brief Sets the very first running thread for the scheduler
 *
 *  @return Void.
 */
void
start_first_running_thread( void )
{
	affirm(scheduler_init);
	tcb_t *first_thread = get_next_run();
	affirm(first_thread);
	first_thread->status = RUNNING;
	running_thread = first_thread;

	affirm(first_thread->owning_task);
	activate_task_memory(first_thread->owning_task);

	/* Disregard stack setup for context switch and call iret_travel */
	uint32_t *kernel_stack_hi = first_thread->kernel_stack_hi;
	uint32_t user_ds = *(--kernel_stack_hi);
	uint32_t user_esp = *(--kernel_stack_hi);
	uint32_t user_eflags = *(--kernel_stack_hi);
	uint32_t user_cs = *(--kernel_stack_hi);
	uint32_t user_eip = *(--kernel_stack_hi);

    /* Before going to user mode, update esp0, so we know where to go back to */
	set_esp0((uint32_t)first_thread->kernel_stack_hi);

	multi_threads = 1;
	iret_travel(user_eip, user_cs, user_eflags, user_esp, user_ds);

	panic("set_first_running_thread does not return");
}


/** @brief Callback for ticks.
 *
 *  @param num_ticks Number of ticks since startup
 *  @return Void.
 *  */
void
scheduler_on_tick( unsigned int num_ticks )
{
	if (!scheduler_init)
		return;

	if (num_ticks % WAIT_TICKS == 0) {
		disable_interrupts();
		add_to_run(running_thread);
		swap_running_thread(get_next_run());
	}
}

/* ------- HELPER FUNCTIONS -------- */

/** @brief Swaps the running thread to to_run.
 *
 *	@pre Interrupts disabled when called.
 *	@param to_run Thread to run next
 *	@param store_at	Queue in which to store old thread in case store_status
 *					is blocked. For any other store status scheduler determines
 *					queue to store thread into.
 *	@param store_status	Status with which to store old thread.
 *	@return Void.
 */
static void
swap_running_thread( tcb_t *to_run )
{
	assert(to_run);
	affirm_msg(scheduler_init, "Scheduler has to be initialized before calling "
			   "swap_running_thread");

	/* No-op if we swap with ourselves */
	if (to_run->tid == running_thread->tid) {
		affirm(to_run->status == RUNNABLE);
		enable_interrupts();
		return;
	}

	tcb_t *running = running_thread;
	to_run->status = RUNNING;
	running_thread = to_run;

	/* Interrupts are enabled inside context switch, once it's safe to do so. */
	switch_threads(running, to_run);

	/* Nothing which must run should be placed after switch_threads as,
	 * when we context switch to a new thread for the first time, the stack
	 * will be setup for it to return directly to the call_fork asm wrapper. */
}

/** @brief Context switch between threads
 *
 *  @param running Currently running thread
 *  @param to_run  Thread to run next
 *  @return Void. Returns in other thread. */
static void
switch_threads(tcb_t *running, tcb_t *to_run)
{
	assert(running && to_run);
	assert(to_run->tid != running->tid);

	/* Let thread know where to come back to on USER->KERN mode switch */
	set_esp0((uint32_t)to_run->kernel_stack_hi);

	context_switch((void **)&(running->kernel_esp), to_run->kernel_esp);
}

/** @brief Whether we have multiple threads registered with the scheduler
 *
 *  @return 1 if there are multiple threads, 0 if not */
int
is_multi_threads( void )
{
	return multi_threads;
}

