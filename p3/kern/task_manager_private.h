/** @brief Internal struct definitions not published in the interface
 */

#ifndef _TASK_MANAGER_PRIVATE_H_
#define _TASK_MANAGER_PRIVATE_H_

typedef struct pcb pcb_t;
typedef struct tcb tcb_t;

/* TODO: Do we even want PCB and TCB to be available to others? */
/** @brief Task control block */
struct pcb {
    void *ptd; // page table directory
    tcb_t *first_thread; // First thread in linked list

    int pid;

    int prepared; // Whether this task's VM has been initialized
};

/** @brief Thread control block */
struct tcb {
    pcb_t *owning_task;
    tcb_t *next_thread; // Embeded linked list of threads from same task

    int tid;

    /* Stack info. Needed for resuming execution.
     * General purpose registers, program counter
     * are stored on stack pointed to by esp. */
    uint32_t *user_esp;
    uint32_t *kernel_esp;
};


#endif /* _TASK_MANAGER_PRIVATE_H_ */

