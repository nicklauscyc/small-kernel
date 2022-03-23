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

/* Timer interrupts every ms, we want to swap every 2 ms. */
#define WAIT_TICKS 2

/* List node to store a tcb in the run queue */
typedef struct run_q_node {
	Q_NEW_LINK(run_q_node) link;
	tcb_t *tcb; // doesn't need to malloc more to hold this pointer
	// TODO change to generic queue so u can just add the tcb in
} run_q_node_t;

/* Boolean for whether initialization has taken place */
int scheduler_init = 0;

/* Global variable for the currently running thread's id */
static int running_tid = 0;

/* List head definition */
Q_NEW_HEAD(list_t, run_q_node);

/* Linked list of run queue. Front element is run */
static list_t run_q;


static void run_next_tcb( void );


/** @brief Gets tid of currently active thread.
 *
 *  @return Tid */
int
get_running_tid( void )
{
	return running_tid;
}

/** @brief Initializes physical allocator family of functions
 *
 *  Must be called once and only once
 *
 *  @param tcb tcb to add to run queue. the first tcb.
 *  @return 0 on success, -1 on error
 */
int
init_scheduler( tcb_t *tcb )
{
	/* Initialize once and only once */
	affirm(!scheduler_init);

	/* Initialize queue head and add the first node into queue */
	Q_INIT_HEAD(&run_q);

	/* Some essential checks that should never fail */
    assert(!Q_GET_TAIL(&run_q));
	assert(!Q_GET_FRONT(&run_q));

	scheduler_init = 1;

	/* add first tcb */
	run_q_node_t *first = malloc(sizeof(run_q_node_t));
	if (!first) {
		return -1;
	}
	first->tcb = tcb;
	Q_INSERT_TAIL(&run_q, first, link);
	assert(Q_GET_FRONT(&run_q) == first);
	return 0;
}

/** @brief Registers thread with scheduler. After this call,
 *         the thread may be executed by the scheduler.
 *
 *  @arg tid Id of thread to register
 *
 *  @return 0 on success, negative value on error */
int
register_thread(uint32_t tid)
{
    tcb_t *tcb = find_tcb(tid);
    if (!tcb)
        return -1;

    if (!scheduler_init) {
        init_scheduler(tcb);
        return - 1;
    }

    //TODO this needs to change to generic tcb in task_manager
    /* add first tcb */
    run_q_node_t *new = malloc(sizeof(run_q_node_t));
    if (!new)
        return -1;

    new->tcb = tcb;
    Q_INSERT_TAIL(&run_q, new, link);
    assert(Q_GET_TAIL(&run_q) == new);

    return 0;
}

void
scheduler_on_tick( unsigned int num_ticks )
{
    if (!scheduler_init)
        return;
    if (num_ticks % WAIT_TICKS == 0) /* Context switch every 2 ms */
        run_next_tcb();
}

/* ------- HELPER FUNCTIONS -------- */

static void
run_next_tcb( void )
{
    // Do nothing if there's only 1 tcb in run queue
    if (Q_GET_FRONT(&run_q) == Q_GET_TAIL(&run_q)) {
        return;
    }

    run_q_node_t *running = Q_GET_FRONT(&run_q);

	/* Put to back of queue */
    Q_REMOVE(&run_q, running, link);
    Q_INSERT_TAIL(&run_q, running, link);

    run_q_node_t *to_run = Q_GET_FRONT(&run_q);
    assert(to_run);
    assert(running);
    assert(1 - running->tcb->tid == to_run->tcb->tid);

    /* Context switch */
    running_tid = to_run->tcb->tid;

    /* Let thread know where to come back to on USER->KERN mode switch */
    set_esp0((uint32_t)to_run->tcb->kernel_stack_hi);

    context_switch((void **)&(running->tcb->kernel_esp),
            to_run->tcb->kernel_esp, to_run->tcb->owning_task->pd);

}

