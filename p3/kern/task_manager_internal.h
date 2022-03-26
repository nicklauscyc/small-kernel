/** @file task_manager_internal.h
 *  @brief Internal struct definitions not published in the interface
 *
 *  @author Nicklaus Choo (nchoo)
 *  @author Andre Nascimento (anascime)
 */

#ifndef _TASK_MANAGER_INTERNAL_H_
#define _TASK_MANAGER_INTERNAL_H_

#include <stdint.h> /* uint32_t */

typedef struct pcb pcb_t;
typedef struct tcb tcb_t;

/* TODO: Do we even want PCB and TCB to be available to others? */
/** @brief Task control block */
struct pcb {
    void *pd; // page directory
    tcb_t *first_thread; // First thread in linked list

    int pid;

    int prepared; // Whether this task's VM has been initialized
};

/** @brief Thread control block */
struct tcb {
    pcb_t *owning_task;
	void *pd; //TODO change this once owning task is set
    tcb_t *next_thread; // Embeded linked list of threads from same task

    int tid;

    /* Stack info. Needed for resuming execution.
     * General purpose registers, program counter
     * are stored on stack pointed to by esp. */
    uint32_t *kernel_stack_hi; /* Highest _writable_ address in kernel stack */
	                           /* that is stack aligned */
    uint32_t *kernel_esp; // needed for context switch
	uint32_t *kernel_stack_lo;
};


#endif /* _TASK_MANAGER_INTERNAL_H_ */

