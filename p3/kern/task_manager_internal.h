/** @file task_manager_internal.h
 *  @brief Internal struct definitions not published in the interface
 *
 *  @author Andre Nascimento (anascime)
 *  @author Nicklaus Choo (nchoo)
 *
 */

#ifndef TASK_MANAGER_INTERNAL_H_
#define TASK_MANAGER_INTERNAL_H_

#include <variable_queue.h> /* Q_NEW_LINK */

/* Possible status of threads */
enum status { RUNNING, RUNNABLE, DESCHEDULED, BLOCKED, DEAD, UNINITIALIZED };
typedef enum status status_t;

/* PCB owned threads queue definition */
Q_NEW_HEAD(owned_threads_queue_t, tcb);

typedef struct pcb pcb_t;
typedef struct tcb tcb_t;

/** @brief Task control block */
struct pcb {
	void *pd; /* page directory */
	owned_threads_queue_t owned_threads; /* list of owned threads */
	uint32_t num_threads; /* number of threads not DEAD */
	pcb_t *next_task; // Embedded list of tasks TODO change to macro
	uint32_t pid; /* Task/process ID */
	int prepared; /* Whether this task's VM has been initialized */
};

/** @brief Thread control block */
struct tcb {
	Q_NEW_LINK(tcb) scheduler_queue; /* Link for queues in scheduler */
	Q_NEW_LINK(tcb) tid2tcb_queue; /* Link for hashmap of TCB queues */
	Q_NEW_LINK(tcb) owning_task_thread_list; /* Link for TCB queue in PCB */
	status_t status; /* Thread's status */
	pcb_t *owning_task; /* PCB of process that owns this thread */
	uint32_t tid; /* Thread ID */

	/* Stack info. Needed for resuming execution.
	* General purpose registers, program counter
	* are stored on stack pointed to by esp. */
	uint32_t *kernel_esp; /* needed for context switch */
	uint32_t *kernel_stack_hi; /* Highest _writable_ address in kernel stack */
	                           /* that is stack aligned */
	uint32_t *kernel_stack_lo; /* Lowest _writable_ kernel stack address */
	                           /* that is stack aligned */

};
#endif /* TASK_MANAGER_INTERNAL_H_ */

