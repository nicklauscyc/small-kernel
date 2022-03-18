/** @brief Module for management of tasks.
 *  Includes context switch facilities. */

#include <task_manager.h>
#include <eflags.h>     /* get_eflags*/
#include <seg.h>        /* SEGSEL_... */
#include <stdint.h>     /* uint32_t, UINT32_MAX */
#include <stddef.h>     /* NULL */
#include <malloc.h>     /* malloc, smemalign, free, sfree */
#include <elf_410.h>    /* simple_elf_t */
#include <page.h>       /* PAGE_SIZE */
#include <cr.h>         /* set_esp0 */
#include <string.h>     /* memset */
#include <assert.h>     /* affirm, assert */
#include <simics.h>     /* sim_reg_process */
#include <iret_travel.h>    /* iret_travel */
#include <memory_manager.h> /* vm_task_new, vm_enable_task */
#include <add_one_atomic.h> /* add_one_atomic */
#include <switch_to.h>  /* switch_to */
#include <scheduler.h>  /* register_new_thread, get_active_thread_id */

#define ELF_IF (1 << 9);
/** @brief Pointer to first task control block. */
pcb_t *pcb_list_start = NULL;

static uint32_t next_tid = 0;
static uint32_t next_pid = 0;

static uint32_t next_task_num ( void );
static uint32_t next_thread_num ( void );

static int find_pcb( uint32_t pid, pcb_t **pcb );
static int new_pcb( uint32_t pid );

static int find_tcb( uint32_t tid, tcb_t **pcb );
static int new_tcb( uint32_t pid, uint32_t tid );

static uint32_t get_user_eflags( void );

/** Switches from currently executing thread to a new one. */
int
thread_switch ( uint32_t from_tid, uint32_t to_tid )
{
    /* Get tcb new and tcb old, and tcb_new->owning_task->pd */
    tcb_t *new_tcbp;
    if (find_tcb(to_tid, &new_tcbp) < 0)
        return -1;

    tcb_t *from_tcbp;
    if (find_tcb(from_tid, &from_tcbp) < 0)
        return -1;

    // TODO: Improve switch_to so that it only updates cr3 on task switch
    switch_to(new_tcbp->kernel_esp, &from_tcbp->kernel_esp, new_tcbp->owning_task->ptd);

    /* Returns on different thread/task. Alternatively, is returned from
     * different thread/task. */

    /** Before returning to whatever we were doing,
     * remember to update kernel stack pointer */
    set_esp0((uint32_t)new_tcbp->kernel_stack_hi);

    return 0;
}

/*  Create new task
 *
 *  @arg out_pid Memory location in which to store task id for new task
 *  @arg elf Elf header for use in allocating new task's memory
 *
 *  @return 0 on success, negative value on failure.
 * */
int
task_new( uint32_t *out_pid, simple_elf_t *elf )
{
    // TODO: Think about preconditions for this.
    // Paging fine, how about making it a critical section?

    // TODO: Check if VM already initialized. Only initialize if it hasn't
    vm_init();

    uint32_t pid = next_task_num();
    uint32_t tid = next_thread_num();

    register_new_thread(tid);

    *out_pid = pid;

    if (new_pcb(pid) < 0)
        return -1;

    /* TODO: Deallocate pcb if this fails */
    if (new_tcb(pid, tid) < 0)
        return -1;


    /* Allocate VM. Stack currently starts at top most address
     * and is PAGE_SIZE long. */
    pcb_t *pcb;
    affirm(find_pcb(pid, &pcb) == 0);

    /* Create new task. Stack is defined here to be the last PAGE_SIZE bytes. */
    vm_task_new(pcb->ptd, elf, UINT32_MAX - PAGE_SIZE + 1, PAGE_SIZE);

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
task_prepare( uint32_t pid )
{
    /* Likely messing up direct mapping of kernel memory, and
     * some instruction after task_prepare is being seen as invalid?*/
    pcb_t *pcb;
    if (find_pcb(pid, &pcb) < 0)
        return -1;

    /* Enable VM */
    vm_enable_task(pcb->ptd);

    pcb->prepared = 1;
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
task_set( uint32_t tid, uint32_t esp, uint32_t entry_point )
{
    tcb_t *tcb;
    affirm(find_tcb(tid, &tcb) == 0);
    pcb_t *pcb = tcb->owning_task;

    if (!pcb->prepared) {
        task_prepare(pcb->pid);
    }


    /* Before going to user mode, update esp0, so we know where to go back to */
    set_esp0((uint32_t)tcb->kernel_stack_hi);

    /* We're currently going directly to entry point. In the future,
     * however, we should go to some "receiver" function which appropriately
     * sets user registers and segment selectors, and lastly RETs to
     * the entry_point. */
    iret_travel(entry_point, SEGSEL_USER_CS, get_user_eflags(),
                esp, SEGSEL_USER_DS);

    /* NOTREACHED */
    panic("iret_travel should not return");
}

// yield -> store gp registers on stack, then switch to some other stack and consume registers
//
// timer interrupt -> all gp stored on stack -> cs, eip and eflags automatically saved in handler stack -> call yield

/** Causes active thread to yield execution. */
void
thread_yield( void )
{

}

/** In case there are no active threads, activates thread with given tid. */
void
thread_activate( uint32_t tid )
{

}


/* ------ HELPER FUNCTIONS ------ */

/** @brief Returns eflags with PL altered to 3 */
static uint32_t
get_user_eflags( void )
{
    uint32_t eflags = get_eflags();

    /* Any IOPL | EFL_IOPL_RING3 == EFL_IOPL_RING3 */
    eflags |= EFL_IOPL_RING3;   /* Set privilege level to user */
    eflags |= EFL_RESV1;        /* Maitain reserved as 1 */
    eflags &= ~(EFL_AC);        /* Disable alignment-checking */
    eflags |= EFL_IF;           /* Enable hardware interrupts */

    return eflags;
}


/** Looks for pcb with given pid.
 *
 *  @arg pid Task id to look for
 *  @arg pcb Memory location where to store pcb, if found.
 *
 *  @return 0 on success, negative value on error. */
static int
find_pcb( uint32_t pid, pcb_t **pcb )
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
find_tcb( uint32_t tid, tcb_t **tcb )
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
new_pcb( uint32_t pid )
{
    /* Ensure alignment of page table directory */
    void *ptd = smemalign(PAGE_SIZE, PAGE_SIZE);
    if (!ptd)
        return -1;

    assert(((uint32_t)ptd & (PAGE_SIZE - 1)) == 0);

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

    return 0;
}

/* TODO: To what extent should this function exist?
 *       When we thread_fork, will we actually use this function? */
static int
new_tcb( uint32_t pid, uint32_t tid )
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
    tcb->kernel_stack_hi = smemalign(PAGE_SIZE, PAGE_SIZE);
    if (!tcb->kernel_stack_hi) {
        free(tcb);
        return -1;
    }

    tcb->kernel_stack_hi = (uint32_t *)(((uint32_t)tcb->kernel_stack_hi) +
            PAGE_SIZE - sizeof(uint32_t));

    tcb->kernel_esp = tcb->kernel_stack_hi;


    return 0;
}

/** Returns next tid */
static uint32_t
next_thread_num ( void )
{
    return add_one_atomic(&next_tid);
}

/** Returns next pid */
static uint32_t
next_task_num ( void )
{
    return add_one_atomic(&next_pid);
}
