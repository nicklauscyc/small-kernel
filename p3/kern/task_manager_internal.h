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
#include <memory_manager.h> /* USER_STR_LEN */

typedef struct pcb pcb_t;
typedef struct tcb tcb_t;

/* PCB owned threads queue definition */
Q_NEW_HEAD(active_threads_list_t, tcb);
Q_NEW_HEAD(vanished_threads_list_t, tcb);

Q_NEW_HEAD(waiting_threads_list_t, tcb);

Q_NEW_HEAD(vanished_child_tasks_list_t, pcb);
Q_NEW_HEAD(active_child_tasks_list_t, pcb);


/** @brief Task/Process Control Block (PCB)
 *
 *  Stores essential information for a task.
 *
 *  @param set_status_vanish_wait_mux Mutex for gainig mutual exclusion to
 *         PCB for when manipulating struct fields in a multi-threaded
 *         environment.
 *  @param pd Page directory address that all threads of this task use.
 *  @param pid Task/Process ID.
 *  @param exit_status Exit status passed on to waiting threads when this task
 *         vanishes.
 *  @param vanished_child_task_list List of vanished child tasks
 *  @param num_vanished_child_tasks Number of child tasks in list of vanished
 *                                  child tasks
 *  @param active_child_tasks_list List of active child tasks
 *  @param num_active_child_tasks Number of child tasks actively running
 *  @param waiting_threads_list List of task threads waiting for child tasks
 *                              to vanish
 *  @param num_waiting_threads Number of threads waiting for child tasks to
 *                             vanish.
 *  @param parent_pcb Pointer to parent task's PCB. If parent task has vanished,
 *                    this is a pointer to the init executable's PCB.
 * 	@param vanished_child_tasks_link Variable queue link for inserting this PCB
 * 	                                 into its parent PCB's vanished child tasks
 * 	                                 list.
 *	@param total_threads Total threads every created
 *	@param active_threads_list List of active threads (not DEAD)
 *	@param num_active_threads Number of threads not DEAD
 *	@param vanished_threads_list List of vanished threads (DEAD)
 *	@param num_vanished_threads Number of vanished threads
 *	@param first_thread_tid Thread ID of task's first thread
 *	@param last_thread Last thread to vanish in task
 *  @param task_link Variable queue link for kernel wide list of all running
 *         tasks.
 */
struct pcb
{
	//mutex_t thread_list_mux; // TODO enable mutex

	mutex_t set_status_vanish_wait_mux; /**< Mutex for PCB when manipulating
	                                      *  struct fields in a multi-threaded
                                          *  environment */
	void *pd; /**<  Page directory address that all threads of this task use. */
	uint32_t pid; /* Task/process ID */
	int exit_status; /* Task exit status */

	char execname[USER_STR_LEN]; // For debugging, executable name

	/* Immediate child tasks that have all their threads vanished */
	vanished_child_tasks_list_t vanished_child_tasks_list;
	uint32_t num_vanished_child_tasks;

	/* Immediate child tasks that have at least 1 active thread */
	active_child_tasks_list_t active_child_tasks_list;
	uint32_t num_active_child_tasks;

	/* List of task threads waiting for child threads to vanish */
	waiting_threads_list_t waiting_threads_list;
	uint32_t num_waiting_threads;

	pcb_t *parent_pcb;
	uint32_t parent_pid;

	/* When the last thread of this task has vanished, this link is used
	 * to put the PCB on its parent task's vanished_child_tasks_list */
	Q_NEW_LINK(pcb) vanished_child_tasks_link;

	uint32_t total_threads; /* total threads every created */

	active_threads_list_t active_threads_list; /* list of owned threads */
	uint32_t num_active_threads; /* number of threads not DEAD */

	vanished_threads_list_t vanished_threads_list;
	uint32_t num_vanished_threads;

	uint32_t first_thread_tid;
	tcb_t *last_thread;
	Q_NEW_LINK(pcb) task_link; /**< Variable queue link for kernel wide list of
	                             *  all running tasks */

	Q_NEW_LINK(pcb) init_pcb_link;



};
/** @brief Thread control block */
struct tcb {
	/* When this thread calls wait(), this link is used to put the TCB on its
	 * owning_task's waiting_threads_list */
	Q_NEW_LINK(tcb) waiting_threads_link;

	Q_NEW_LINK(tcb) scheduler_queue; /* Link for queues in scheduler */
	Q_NEW_LINK(tcb) tid2tcb_queue; /* Link for hashmap of TCB queues */

	/* This link is for the owning task's active_threads_list or
	 * vanished_threads_list. Using the same link name enforces that
	 * a TCB can be put on at most one type of list at any one time */
	Q_NEW_LINK(tcb) task_thread_link; /* Link for TCB queue in PCB */

    pcb_t *collected_vanished_child; /* for use on wait */

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


	/* Info for syscalls */
	/* When this thread should be woken up (if it is sleeping) */
	uint32_t sleep_expiry_date;

	/* Software exception handler info. Handler/stack NULL if not registered */
	uint32_t swexn_handler;
	uint32_t swexn_stack;
	void	*swexn_arg;
	int		 has_swexn_handler;
};
#endif /* TASK_MANAGER_INTERNAL_H_ */

