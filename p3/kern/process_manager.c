/** Context switch facilities */

#include <stdint.h>
#include <stddef.h>
#include <malloc.h>

// TODO: Have this be defined in an internal .h file
// and be shared with mem_manager.c
#define PAGE_ENTRY_SIZE 64
#define PAGE_TABLE_SIZE (1024 * PAGE_ENTRY_SIZE)

/* Page directory and page table coincidentally have
 * the same size. */
#define PAGE_DIRECTORY_SIZE PAGE_TABLE_SIZE

typedef struct pcb pcb_t;
typedef struct tcb tcb_t;

struct pcb {
    void *ptd_start; // Start of page table directory
    tcb_t *first_thread; // First thread in linked list

};

struct tcb {
    pcb_t *owning_task;
    tcb_t *next_thread; // Embeded linked list of threads from same process

};

pcb_t pcb_list_start;

/* Create new process (memory + pcb and tcb) */
int
new_task( simple_elf_t *elf )
{
    // TODO: Affirm kernel_mode

    /* TODO: Paging might be enabled, check for that and disable it. */

    // TODO: Add to pcb_list, for now support 1 task (just set pcb_list_start)

    /*-- Checkpoint 1 impl start --*/
    pcb_list_start->ptd_start = malloc(
}

