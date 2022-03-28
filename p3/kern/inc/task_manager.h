/** @brief Facilities for task creation and management.
 *  Also provides context switching functionallity. */

#ifndef TASK_MANAGER_H_
#define TASK_MANAGER_H_

#include <stdint.h>         /* uint32_t */
#include <elf_410.h>        /* simple_elf_t */
#include <variable_queue.h> /* Q_NEW_LINK */

/* FIXME: Which of these do we really need? */
enum status { RUNNING, RUNNABLE, DESCHEDULED, BLOCKED, DEAD, UNINITIALIZED };
typedef enum status status_t;

typedef struct pcb pcb_t;
typedef struct tcb tcb_t;

/** @brief Task control block */
struct pcb {
    void *pd; // page directory
    tcb_t *first_thread; // First thread in linked list
    pcb_t *next_task; // Embedded list of tasks

    uint32_t pid;

    int prepared; // Whether this task's VM has been initialized
};

/** @brief Thread control block */
struct tcb {
	Q_NEW_LINK(tcb) scheduler_queue;
	Q_NEW_LINK(tcb) tid2tcb_queue;

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


tcb_t *find_tcb( uint32_t tid );
pcb_t *find_pcb( uint32_t pid );
int create_pcb( uint32_t *pid, void *pd );
int create_tcb( uint32_t pid , uint32_t *tid );

int create_task( uint32_t *pid, uint32_t *tid, simple_elf_t *elf );
int activate_task_memory( uint32_t pid );
void task_set_active( uint32_t tid, uint32_t esp, uint32_t entry_point );

#endif /* TASK_MANAGER_H_ */
