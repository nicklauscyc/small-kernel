/** @brief Module for management of tasks.
 *  Includes context switch facilities. */

#include <task_manager.h>
#include <limits.h>     /* UINT_MAX */
#include <eflags.h>     /* get_eflags*/
#include <seg.h>        /* SEGSEL_... */
#include <stdint.h>     /* uint32_t */
#include <stddef.h>     /* NULL */
#include <malloc.h>     /* malloc, smemalign, free, sfree */
#include <elf_410.h>    /* simple_elf_t */
#include <page.h>       /* PAGE_SIZE */
#include <string.h>     /* memset */
#include <assert.h>     /* affirm, assert */
#include <simics.h>     /* sim_reg_process */
#include <iret_travel.h>    /* iret_travel */
#include <memory_manager.h> /* vm_task_new, vm_enable_task */

/** @brief Pointer to first task control block. */
pcb_t *pcb_list_start = NULL;

static int find_pcb( int pid, pcb_t **pcb );
static int new_pcb( int pid );

static int find_tcb( int tid, tcb_t **pcb );
static int new_tcb( int pid, int tid );

static uint32_t get_user_eflags( void );

/*  Create new task
 *
 *  @arg pid Task id for new task
 *  @arg tid Thread id for new thread
 *  @arg elf Elf header for use in allocating new task's memory
 *
 *  @return 0 on success, negative value on failure.
 * */
int
task_new( int pid, int tid, simple_elf_t *elf )
{
    // TODO: Think about preconditions for this.
    // Paging fine, how about making it a critical section?

    lprintf("creating pcb");

    if (new_pcb(pid) < 0)
        return -1;

    lprintf("creating tcb");

    /* TODO: Deallocate pcb if this fails */
    if (new_tcb(pid, tid) < 0)
        return -1;


    /* Allocate VM. Stack currently starts at top most address
     * and is PAGE_SIZE long. */
    pcb_t *pcb;
    lprintf("finding pcb ");
    affirm(find_pcb(pid, &pcb) == 0);
    lprintf("creating vm task_new ");
    vm_task_new(pcb->ptd, elf, UINT_MAX, PAGE_SIZE);

    lprintf("registering process w/ simics");
#ifndef NDEBUG
    /* Register this task with simics for better debugging */
    sim_reg_process(pcb->ptd, elf->e_fname);
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
    /* Likely messing up direct mapping of kernel memory, and
     * some instruction after task_prepare is being seen as invalid?*/
    pcb_t *pcb;
    if (find_pcb(pid, &pcb) < 0)
        return -1;

    lprintf("before enabling vm");
    /* Enable VM */
    vm_enable_task(pcb->ptd);
    lprintf("after enabling vm");

    return 0;
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
 *  @arg esp Stack pointer
 *  @arg entry_point First program instruction
 *
 *  @return Never returns.
 *  */
void
task_set( int tid, uint32_t esp, uint32_t entry_point )
{
    tcb_t *tcb;
    affirm(find_tcb(tid, &tcb) == 0);
    pcb_t *pcb = tcb->owning_task;

    if (!pcb->prepared) {
        task_prepare(pcb->pid);
    }

    lprintf("before iret travel");

    /* We're currently going directly to entry point. In the future,
     * however, we should go to some "receiver" function which appropriately
     * sets user registers and segment selectors, and lastly RETs to
     * the entry_point. */
    iret_travel(SEGSEL_USER_DS, esp, get_user_eflags(),
                SEGSEL_USER_CS, entry_point);

    /* NOTREACHED */
    affirm(0);
}

/* Aka context_switch */
void
task_switch( int pid )
{
    (void)pid;
    /* TODO: Unimplemented */
}


/* ------ HELPER FUNCTIONS ------ */

/** @brief Returns eflags with PL altered to 3 */
static uint32_t
get_user_eflags( void )
{
    /* Any IOPL | EFL_IOPL_RING3 == EFL_IOPL_RING3 */
    return get_eflags() | EFL_IOPL_RING3;
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
find_tcb( int tid, tcb_t **tcb )
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
    lprintf("smemalign");
    /* Ensure alignment of page table directory */
    void *ptd = smemalign(PAGE_SIZE, PAGE_SIZE);
    lprintf("after smemalign");
    if (!ptd)
        return -1;

    lprintf("assert");
    assert(((uint32_t)ptd & (PAGE_SIZE - 1)) == 0);

    lprintf("malloc");
    pcb_t *pcb = malloc(sizeof(pcb_t));
    if (!pcb) {
        sfree(ptd, PAGE_SIZE);
        return -1;
    }

    lprintf("memset");
    /* Ensure all entries are 0 and therefore not present */
    memset(ptd, 0, PAGE_SIZE);

    pcb->ptd = ptd;
    pcb->pid = pid;
    pcb->first_thread = NULL;
    pcb->prepared = 0;

    pcb_list_start = pcb;

    return 0;
}

/* TODO: To what extent should this function exist?
 *       When we thread_fork, will we actually use this function? */
static int
new_tcb( int pid, int tid )
{
    pcb_t *owning_task;
    if (find_pcb(pid, &owning_task) < 0) {
        return -1;
    }

    tcb_t *tcb = malloc(sizeof(tcb_t));
    if (!tcb) {
        return -1;
    }

    owning_task->first_thread = tcb;

    /* Set tcb/pcb values  */
    tcb->owning_task = owning_task;
    tcb->next_thread = NULL;
    tcb->tid = tid;

    return 0;
}
