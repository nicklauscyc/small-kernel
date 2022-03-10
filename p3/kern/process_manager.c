/** @brief Module for management of processes.
 *  Includes context switch facilities. */

#include <stdint.h>
#include <stddef.h>
#include <malloc.h>
#include <elf/elf_410.h>
#include <page.h> /* PAGE_SIZE */
#include <string.h> /* memset */
#include <simics.h> /* sim_reg_process */

// TODO: Have all these be redefined in an internal .h file
#define WORD_SIZE 4
#define PAGE_TABLE_ENTRIES (PAGE_SIZE / WORD_SIZE)
#define PAGE_DIRECTORY_ENTRIES PAGE_TABLE_ENTRIES
#define USER_MODE 0
#define KERN_MODE 1

typedef struct pcb pcb_t;
typedef struct tcb tcb_t;

/** @brief Task control block */
struct pcb {
    void *ptd; // page table directory
    tcb_t *first_thread; // First thread in linked list

    int pid;

    int prepared; // Whether this task's VM has been initialized
};

/** @brief Thread control block */
// TODO: Add necessary registers + program counter
struct tcb {
    pcb_t *owning_task;
    tcb_t *next_thread; // Embeded linked list of threads from same process

    int tid;
    int mode; // USER_MODE or KERN_MODE, for bookkeeping only

    /* Stack info. Needed for resuming execution.
     * General purpose registers, program counter
     * are stored on stack pointed to by esp. */
    int esp;
};

pcb_t *pcb_list_start = NULL;

static int find_pcb( int pid, pcb_t **pcb );

static int new_pcb( int pid, int tid );

//int
//set_task_esp( int ss, int esp )
//{
//}

/*  Create new process
 *
 *  @arg pid Task id for new task
 *  @arg tid Thread id for new thread
 *  @arg elf Elf header for use in allocating new task's memory
 *
 *  @return 0 on success, negative value on failure.
 * */
int
new_task( int pid, int tid, simple_elf_t *elf )
{
    // TODO: Think about preconditions for this.
    // Paging fine, how about making it a critical section?

    if (new_pcb(pid) < 0)
        return -1;

    /* TODO: Deallocate pcb if this fails */
    if (new_tcb(pid, tid) < 0)
        return -1;

    /* Allocate VM */
    pcb_t pcb;
    affirm(find_pcb(pid, &pcb) == 0);
    vm_new_task(pcb.ptd, elf);

#ifndef NDEBUG
    /* Register this process with simics for better debugging */
    sim_reg_process(pcb.ptd, elf->e_fname);
#endif

    return 0;
}

/** NOTE: Not to be used in context-switch, only when running task
 *  for the first time
 *
 *  Enables virtual memory of task. Use this before transplanting data
 *  into task's memory.
 *  */
int
task_prepare( int pid )
{
    pcb_t pcb;
    if (find_pcb(pid, &pcb) < 0)
        return -1;

    /* Enable VM */
    vm_enable_task(pcb.ptd);

    return 0;
}

uint32_t
get_user_eflags( void )
{
    /* Any IOPL | EFL_IOPL_RING3 == EFL_IOPL_RING3 */
    return get_eflags() | EFL_IOPL_RING3;
}

/** NOTE: Not to be used in context-switch, only when running task
 *  for the first time
 *
 *  Should only ever be called once, and after task has been initialized
 *  after a call to new_task.
 *  The caller is supposed to install memory on the new task before
 *  calling this function. Stack pointer should be appropriately set
 *  if any arguments have been loaded on stack.
 *
 *  @arg tid Id of thread to run
 *  @arg stack_lo Stack pointer
 *  @arg entry_point First program instruction
 *
 *  @return Never returns.
 *  */
void
task_set( int tid, uint32_t stack_lo, uint32_t entry_point )
{
    tcb_t tcb;
    affirm(find_tcb(tid, &tcb) == 0);
    pcb_t *pcb = tcb->owning_task;

    if (!pcb->prepared) {
        task_prepare(pcb->pid);
    }

    iret_travel(SEGSEL_USER_DS, stack_lo, get_user_eflags(),
                SEGSEL_USER_CS, entry_point);

    /* NOTREACHED */
    affirm(0);
}

/* Aka context_switch */
int
task_switch( int pid )
{
    /* TODO: Unimplemented */
    return -1;
}

/** Looks for pcb with given pid.
 *
 *  @arg pid Task id to look for
 *  @arg pcb Memory location where to store pcb, if found.
 *
 *  @return 0 on success, negative value on error. */
static int
find_pcb( int pid, pcb_t **pcb )
{
    // TODO: Actually implement the search
    if (!pcb_list_start)
        return -1;
    *pcb = pcb_list_start;
    return 0;
}

/** Looks for tcb with given tid.
 *
 *  @arg tid Thread id to look for
 *  @arg tcb Memory location where to store tcb, if found.
 *
 *  @return 0 on success, negative value on error. */
static int
find_pcb( int pid, pcb_t **pcb )
{
    // TODO: Actually implement the search
    if (!pcb_list_start)
        return -1;
    *tcb = pcb_list_start->first_thread;
    return 0;
}

/* Initializes new pcb.
 *
 * TODO: Should we initialize a TCB here as well?
 *       Does it make sense for a task with no threads to exist? */
static int
new_pcb( int pid )
{
    /* Ensure alignment of page table directory */
    void *ptd = smemalign(PAGE_SIZE, PAGE_SIZE);
    if (!ptd)
        return -1;

    assert(ptd & (PAGE_SIZE - 1) == 0);

    pcb_t *pcb = malloc(sizeof(pcb_t));
    if (!pcb) {
        sfree(ptd, PAGE_SIZE);
        return -1;
    }

    /* Ensure all entries are 0 and therefore not present */
    memset(ptd, 0, PAGE_SIZE);

    pcb->ptd = ptd;
    pcb->pid = pid;
    pcb->first_thread = NULL;
    pcb->prepared = 0;

    pcb_list_start = pcb;
}

/* TODO: To what extent should this function exist?
 *       When we thread_fork, will we actually use this function? */
static int
new_tcb( int pid, int tid )
{
    tcb_t *tcb = malloc(sizeof(tcb_t));
    if (!tcb) {
        return -1;
    }

    /* Set tcb/pcb values  */
    tcb->owning_task = new_pcb;
    tcb->next_thread = NULL;
    tcb->tid = tid;
    tcb->mode = USER_MODE;

    pcb_t *owning_task;
    if (find_pcb(pid, &owning_task) < 0) {
        free(tcb);
        return -1;
    }
    owning_task->first_thread = tcb;

}
