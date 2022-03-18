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
#include <memory_manager.h> /* get_new_page_table, vm_enable_task */

#define ELF_IF (1 << 9);
/** @brief Pointer to first task control block. */
pcb_t *pcb_list_start = NULL;

static int find_pcb( int pid, pcb_t **pcb );
static int new_pcb( int pid, void *pd );

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
get_new_task_data_structures( int pid, int tid, simple_elf_t *elf )
{
    // TODO: Think about preconditions for this.
    // Paging fine, how about making it a critical section?

	/* Allocates physical memory to a new page table and enables VM */
    /* Ensure alignment of page table directory */
    /* Create new task. Stack is defined here to be the last PAGE_SIZE bytes. */
    void *pd = get_new_pd(elf, UINT32_MAX - PAGE_SIZE + 1, PAGE_SIZE);
	if (!pd) {
		return -1;
	}

    if (new_pcb(pid, pd) < 0)
        return -1;

    /* TODO: Deallocate pcb if this fails */
    if (new_tcb(pid, tid) < 0)
        return -1;


    /* Allocate VM. Stack currently starts at top most address
     * and is PAGE_SIZE long. */
    pcb_t *pcb;
    affirm(find_pcb(pid, &pcb) == 0);


#ifndef NDEBUG
    /* Register this task with simics for better debugging */
    sim_reg_process(pcb->pd, elf->e_fname);
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

    /* Enable VM */
    vm_enable_task(pcb->pd);

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
task_set( int tid, uint32_t esp, uint32_t entry_point )
{
    tcb_t *tcb;
    affirm(find_tcb(tid, &tcb) == 0);
    pcb_t *pcb = tcb->owning_task;

    if (!pcb->prepared) {
        task_prepare(pcb->pid);
    }


    /* Before going to user mode, update esp0, so we know where to go back to */
    set_esp0((uint32_t)tcb->kernel_esp);

    /* We're currently going directly to entry point. In the future,
     * however, we should go to some "receiver" function which appropriately
     * sets user registers and segment selectors, and lastly RETs to
     * the entry_point. */
    iret_travel(entry_point, SEGSEL_USER_CS, get_user_eflags(),
                esp, SEGSEL_USER_DS);

    /* NOTREACHED */
    panic("iret_travel should not return");
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
    uint32_t eflags = get_eflags();

    /* Any IOPL | EFL_IOPL_RING3 == EFL_IOPL_RING3 */
    eflags |= EFL_IOPL_RING3; /* Set privilege level to user */
    eflags |= EFL_RESV1; /* Maitain reserved as 1 */
    eflags &= ~(EFL_AC); /* Disable alignment-checking */
    eflags |= EFL_IF; /* TODO:(should we???) Enable hardware interrupts */

    return eflags;
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
new_pcb( int pid, void *pd )
{
    pcb_t *pcb = malloc(sizeof(pcb_t));
    if (!pcb) {
        sfree(pd, PAGE_SIZE);
        return -1;
    }

    pcb->pd = pd;
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
    tcb->kernel_esp = smemalign(PAGE_SIZE, PAGE_SIZE);
    tcb->kernel_esp = (uint32_t *)(((uint32_t)tcb->kernel_esp) +
            PAGE_SIZE - sizeof(uint32_t));

    if (!tcb->kernel_esp) {
        free(tcb);
        return -1;
    }

    return 0;
}
