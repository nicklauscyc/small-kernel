
#include <inc/task_manager.h> /* tcb_t */

#include <variable_queue.h> /* Q_NEW_LINK() */
#include <assert.h> /* affirm() */
#include <malloc.h> /* malloc() */
#include <stdint.h> /* uint32_t */
#include <context_switch.h> /* context_switch() */
#include <x86/cr.h> /* get_esp0() */

/* Boolean for whether initialization has taken place */
int scheduler_init = 0;

/* List node to store a tcb in the run queue */
typedef struct run_q_node {
	Q_NEW_LINK(run_q_node) link;
	tcb_t *tcb; // doesn't need to malloc more to hold this pointer
	// TODO change to generic queue so u can legit just add the tcb in
} run_q_node_t;

/* List head definition */
Q_NEW_HEAD(list_t, run_q_node);

/* Linked list of run queue. Front element is run */
static list_t run_q;

int init_scheduler( tcb_t *tcb );
void add_tcb_to_run_queue(tcb_t *tcb);
void run_next_tcb( void );


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

void
add_tcb_to_run_queue(tcb_t *tcb)
{
	if (!scheduler_init) {
		init_scheduler(tcb);
		return;
	}
	//TODO this needs to change to generic tcb in task_manager
	/* add first tcb */
	run_q_node_t *new = malloc(sizeof(run_q_node_t));
	affirm(new);

	new->tcb = tcb;
	Q_INSERT_TAIL(&run_q, new, link);
	assert(Q_GET_TAIL(&run_q) == new);
}

void
run_next_tcb( void )
{
	run_q_node_t *running = Q_GET_FRONT(&run_q);

	/* Put to back of queue */
	Q_REMOVE(&run_q, running, link);
	Q_INSERT_TAIL(&run_q, running, link);

	/* Get next tcb to run */
	run_q_node_t *to_run = Q_GET_FRONT(&run_q);
	assert(to_run != running);
	assert(to_run);
	assert(running);
	assert(running->tcb->tid == 0);
	assert(to_run->tcb->tid == 1);

    //assert(1 - running->tcb->tid == to_run->tcb->tid);

#include <simics.h>
	lprintf("context switching");
	/* Context switch */
	lprintf("to_run->tcb->kernel_esp:%p", to_run->tcb->kernel_esp);
	context_switch((void **)&(running->tcb->kernel_esp),
	               to_run->tcb->kernel_esp, to_run->tcb->pd);
	lprintf("after context switching");

}

