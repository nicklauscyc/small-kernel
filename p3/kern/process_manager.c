/** Context switch facilities */

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
};

/** @brief Thread control block */
// TODO: Add necessary registers + program counter
struct tcb {
    pcb_t *owning_task;
    tcb_t *next_thread; // Embeded linked list of threads from same process

    int tid;
    int mode; // USER_MODE or KERN_MODE, for bookkeeping only

    /* Stack info. Needed for resuming execution.
     * General purpose registers, program counter and
     * data segment selectors are stored on stack pointed
     * to by ss:esp. */
    int ss;
    int esp;
};

pcb_t *pcb_list_start = NULL;

static int find_pcb( int pid, pcb_t **pcb );

static int new_pcb( int pid, int tid );

int
set_task_esp( int ss, int esp )
{
}

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
 *  Sets task as active, aka updates the virtual memory appropriately.
 *  However, this DOES NOT run a task or switch from kernel to user mode.
 *  */
int
set_active_task( int pid )
{
    pcb_t pcb;
    if (find_pcb(pid, &pcb) < 0)
        return -1;

    /* Enable VM */
    vm_enable_task(pcb.ptd);

    return 0;
}

/** NOTE: Not to be used in context-switch, only when running task
 *  for the first time
 *
 *  Once loader has transplanted all program data into virtual memory
 *  and met any other requirements (such as saving registers on stack)
 *  this function will run the task and never return. */
int
run_task( int tid )
{
    /* TODO: This should set up registers and start running the task. */
    /* May assume all memory has been setup properly by loader.  */
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

/* Initializes new pcb. */
static int
new_pcb( int pid, int tid )
{
    /* Ensure alignment of page table directory */
    void *ptd = smemalign(PAGE_SIZE, PAGE_SIZE);
    if (!ptd)
        return -1;

    assert(ptd & (PAGE_SIZE - 1) == 0);

    pcb_t *new_pcb = malloc(sizeof(pcb_t));
    if (!new_pcb) {
        sfree(ptd, PAGE_SIZE);
        return -1;
    }

    /* Ensure all entries are 0 and therefore not present */
    memset(ptd, 0, PAGE_SIZE);

    new_pcb->ptd = ptd;
    new_pcb->pid = pid;
    new_pcb->first_thread = NULL;

    pcb_list_start = new_pcb;
}

int
new_tcb( int pid, int tid )
{
    tcb_t *new_tcb = malloc(sizeof(tcb_t));
    if (!new_tcb) {
        return -1;
    }

    /* Set tcb/pcb values  */
    new_tcb->owning_task = new_pcb;
    new_tcb->next_thread = NULL;
    new_tcb->tid = tid;
    new_tcb->mode = USER_MODE;

    pcb_t *owning_task;
    if (find_pcb(pid, &owning_task) < 0) {
        free(new_tcb);
        return -1;
    }
    owning_task->first_thread = new_tcb;

}

/** Call to run task.
 *  Should only ever be called once, and after a call to new_task.
 *  The caller is supposed to install memory on the new task before
 *  calling this function. Stack pointer should be appropriately set
 *  if any arguments have been loaded on stack.
 *
 *  @arg tid Thread id of thread to run
 *  @arg ss  Stack segment selector
 *  @arg esp Stack pointer
 *  @arg eip Task entry point
 *
 *  @return Doesn't return.
 *  */
void
run_task( int tid, int ss, int esp, int eip )
{

}
