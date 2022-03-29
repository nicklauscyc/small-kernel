/** @file task_manager_internal.h
 *  @brief Internal struct definitions not published in the interface
 *
 *  @author Nicklaus Choo (nchoo)
 *  @author Andre Nascimento (anascime)
 */

#ifndef TASK_MANAGER_INTERNAL_H_
#define TASK_MANAGER_INTERNAL_H_

enum status { RUNNING, RUNNABLE, DESCHEDULED, BLOCKED, DEAD, UNINITIALIZED };
typedef enum status status_t;

/* pcb_queue definition */
Q_NEW_HEAD(owned_threads_queue_t, tcb);

typedef struct pcb pcb_t;
typedef struct tcb tcb_t;

/** @brief Task control block */
struct pcb {
    void *pd; // page directory
	owned_threads_queue_t owned_threads; // list of owned threads
	uint32_t num_threads;

    pcb_t *next_task; // Embedded list of tasks TODO change to macro

    uint32_t pid;

    int prepared; // Whether this task's VM has been initialized
};

/** @brief Thread control block */
struct tcb {
	Q_NEW_LINK(tcb) scheduler_queue;
	Q_NEW_LINK(tcb) tid2tcb_queue;
	Q_NEW_LINK(tcb) owning_task_thread_list;


    status_t status;

    pcb_t *owning_task;
    tcb_t *next_thread; // Embedded linked list of threads from same task

    uint32_t tid;

    /* Stack info. Needed for resuming execution.
     * General purpose registers, program counter
     * are stored on stack pointed to by esp. */
    uint32_t *kernel_stack_hi; /* Highest _writable_ address in kernel stack */
	                           /* that is stack aligned */
    uint32_t *kernel_esp; // needed for context switch
	uint32_t *kernel_stack_lo;
};


#endif /* TASK_MANAGER_INTERNAL_H_ */

