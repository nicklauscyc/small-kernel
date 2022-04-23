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
#include <scheduler.h> /* status_t */
#include <lib_thread_management/mutex.h> /* mutex_t */

/* PCB owned threads queue definition */
Q_NEW_HEAD(owned_threads_queue_t, tcb);

typedef struct pcb pcb_t;
typedef struct tcb tcb_t;

/** @brief Task control block */
struct pcb {
	mutex_t set_status_vanish_wait_mux;
	//mutex_t thread_list_mux; // TODO enable mutex
	void *pd; /* page directory */
	owned_threads_queue_t owned_threads; /* list of owned threads */
	uint32_t num_threads; /* number of threads not DEAD */
	Q_NEW_LINK(pcb) task_link; // Embedded list of tasks
	uint32_t pid; /* Task/process ID */
	int prepared; /* Whether this task's VM has been initialized */
	int exit_status; /* Task exit status */
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


	/* When this thread should be woken up (if it is sleeping) */
	uint32_t sleep_expiry_date;

	/* Software exception handler info. Handler/stack NULL if not registered */
	uint32_t swexn_handler;
	uint32_t swexn_stack;
	void	*swexn_arg;
	int		 has_swexn_handler;
};
#endif /* TASK_MANAGER_INTERNAL_H_ */

