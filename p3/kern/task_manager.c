/** @brief Module for management of tasks.
 *  Includes context switch facilities. */

#include <task_manager.h>

#include <scheduler.h>  /* add_tcb_to_run_queue() */
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
#include <logger.h>     /* log */
#include <iret_travel.h>    /* iret_travel */
#include <memory_manager.h> /* get_new_page_table, vm_enable_task */
#include <lib_thread_management/hashmap.h> /* map_* functions */
#include <lib_thread_management/add_one_atomic.h>

#define ELF_IF (1 << 9);

static uint32_t get_unique_tid( void );
static uint32_t get_unique_pid( void );
static uint32_t get_user_eflags( void );

static pcb_t *first_pcb = NULL;

/** @brief Next pid to be assigned. Only to be updated by get_unique_pid */
static uint32_t next_pid = 0;
/** @brief Next tid to be assigned. Only to be updated by get_unique_tid */
static uint32_t next_tid = 0;

/*  Creates a task
 *
 *  @arg pid Pointer where task id for new task is stored
 *  @arg tid Pointer where thread id for new thread is stored
 *  @arg elf Elf header for use in allocating new task's memory
 *
 *  @return 0 on success, negative value on failure.
 * */
int
create_task( uint32_t *pid, uint32_t *tid, simple_elf_t *elf )
{
    // TODO: Think about preconditions for this.
    // Paging fine, how about making it a critical section?

	/* Allocates physical memory to a new page table and enables VM */
    /* Ensure alignment of page table directory */
    /* Create new task. Stack is defined here to be the last PAGE_SIZE bytes. */
    void *pd = new_pd_from_elf(elf, UINT32_MAX - PAGE_SIZE + 1, PAGE_SIZE);
	if (!pd) {
		return -1;
	}

    if (create_pcb(pid, pd) < 0) {
        sfree(pd, PAGE_SIZE);
        return -1;
    }

    pcb_t *pcb = find_pcb(*pid);
    affirm(pcb);

    if (create_tcb(*pid, tid) < 0) {
        // TODO: Delete pcb, and return -1
        sfree(pd, PAGE_SIZE);
        return -1;
    }

    /* Let scheduler know it can now run this thread */
    register_thread(*tid);

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
activate_task_memory( uint32_t pid )
{
    /* Likely messing up direct mapping of kernel memory, and
     * some instruction after task_prepare is being seen as invalid?*/
    pcb_t *pcb;
    if ((pcb = find_pcb(pid)) == NULL)
        return -1;

    /* Enable VM */
    vm_enable_task(pcb->pd);

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
task_set_active( uint32_t tid, uint32_t esp, uint32_t entry_point )
{
    tcb_t *tcb;
    affirm((tcb = find_tcb(tid)) != NULL);
    pcb_t *pcb = tcb->owning_task;

    // TODO: Remove this check?
    if (!pcb->prepared) {
        activate_task_memory(pcb->pid);
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

/** Looks for pcb with given pid.
 *
 *  @arg pid Task id to look for
 *
 *  @return Pointer to pcb on success, NULL on failure */
pcb_t *
find_pcb( uint32_t pid )
{
    /* TODO: Might want to create a pcb hashmap. Is it worth it? */
    pcb_t *res = first_pcb;
    while (res && res->pid != pid)
        res = res->next_task;
    return res;
}

/** Looks for tcb with given tid.
 *
 *  @arg tid Thread id to look for
 *
 *  @return Pointer to tcb on success, NULL on failure */
tcb_t *
find_tcb( uint32_t tid )
{
    return (tcb_t *)map_get(tid);
}

/** @brief Initializes new pcb, and corresponding tcb.
 *
 *  @arg pid Pointer to where pid should be stored
 *  @arg pd  Pointer to page directory for new task
 *
 *  @return 0 on success, negative value on error
 * */
int
create_pcb( uint32_t *pid, void *pd )
{
    pcb_t *pcb = smalloc(sizeof(pcb_t));
    if (!pcb)
        return -1;

    *pid = get_unique_pid();
    pcb->pid = *pid;
    pcb->pd = pd;
    pcb->prepared = 0;
    pcb->first_thread = NULL;

    /* Add to pcb linked list*/
    pcb->next_task = first_pcb;
    first_pcb = pcb;

    return 0;
}

/** @brief Initializes new tcb. Does not add thread to scheduler.
 *         This should be done by whoever creates this thread.
 *
 *  @arg pid    Id of owning task
 *  @arg tid    Pointer to where id of new thread will be stored
 *
 *  @return 0 on success, negative value on failure
 * */
int
create_tcb( uint32_t pid, uint32_t *tid )
{
    pcb_t *owning_task;
    if ((owning_task = find_pcb(pid)) == NULL)
        return -1;

    tcb_t *tcb = smalloc(sizeof(tcb_t));
    if (!tcb) {
        return -1;
    }

    *tid = get_unique_tid();
    tcb->tid = *tid;
	// TODO when do we use smalloc and when do we use smemalign
	tcb->kernel_stack_lo = smalloc(PAGE_SIZE);
    if (!tcb->kernel_stack_lo) {
        sfree(tcb, sizeof(tcb_t));
        return -1;
    }

    tcb->status = UNINITIALIZED;

    /* Add to owning task's list of threads */
    tcb->owning_task = owning_task;
    tcb->next_thread = tcb->owning_task->first_thread;
    tcb->owning_task->first_thread = tcb;

    log("Inserting thread with tid %lu", tcb->tid);
	map_insert(tcb->tid, (void *)tcb);

	/* memset the whole thing, TODO delete this in future, only good for
	 * debugging when printing the whole stack
	 */
	memset(tcb->kernel_stack_lo, 0, PAGE_SIZE);

	log("tid[%lu]: tcb->stack_lo:%p",tcb->tid,
	        tcb->kernel_stack_lo);

    tcb->kernel_esp = tcb->kernel_stack_lo;
    tcb->kernel_esp = (uint32_t *)(((uint32_t)tcb->kernel_esp) +
            PAGE_SIZE - sizeof(uint32_t));

	// store tid at highest kernel stack address
	*(tcb->kernel_esp) = tcb->tid;
	tcb->kernel_stack_hi = tcb->kernel_esp;
	// tcb->kernel_esp--; // I feel like this is not needed cuz u will decrement
	// esp then store under all cases

    return 0;
}

/* ------ HELPER FUNCTIONS ------ */

/** @brief Returns eflags with PL altered to 3 */
static uint32_t
get_user_eflags( void )
{
    uint32_t eflags = get_eflags();

    /* Any IOPL | EFL_IOPL_RING3 == EFL_IOPL_RING3 */
    eflags |= EFL_IOPL_RING0; /* Set privilege level to user */
    eflags |= EFL_RESV1;      /* Maitain reserved as 1 */
    eflags &= ~(EFL_AC);      /* Disable alignment-checking */
    eflags |= EFL_IF;         /* Enable hardware interrupts */

    return eflags;
}

/** @brief Returns a unique pid. */
static uint32_t
get_unique_pid( void )
{
    return add_one_atomic(&next_pid);
}

/** @brief Returns a unique tid. */
static uint32_t
get_unique_tid( void )
{
    return add_one_atomic(&next_tid);
}
