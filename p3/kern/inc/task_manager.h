/** @brief Facilities for task creation and management.
 *  Also provides context switching functionallity. */

#ifndef _TASK_MANAGER_H
#define _TASK_MANAGER_H

#include <stdint.h> /* uint32_t */
#include <elf_410.h> /* simple_elf_t */

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
    int esp;
};

int task_new( int pid, int tid, simple_elf_t *elf );
int task_prepare( int pid );
void task_set( int tid, uint32_t esp, uint32_t entry_point );
void task_switch( int pid );

#endif /* _TASK_MANAGER_H */
