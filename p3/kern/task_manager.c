/** @brief Module for management of tasks.
 *	Includes context switch facilities. */

// TODO maybe a function such as is_legal_pcb, is_legal_tcb for invariant checks?
//
#include <task_manager.h>
#include <task_manager_internal.h>
#include <assert.h> /* affirm() */

#include <scheduler.h>	/* add_tcb_to_run_queue() */
#include <eflags.h>	/* get_eflags*/
#include <seg.h>	/* SEGSEL_... */
#include <stdint.h>	/* uint32_t, UINT32_MAX */
#include <stddef.h>	/* NULL */
#include <malloc.h>	/* malloc, smemalign, free, sfree */
#include <elf_410.h>	/* simple_elf_t */
#include <page.h>	/* PAGE_SIZE */
#include <cr.h>		/* set_esp0 */
#include <string.h>	/* memset */
#include <assert.h>	/* affirm, assert */
#include <simics.h>	/* sim_reg_process */
#include <logger.h>	/* log */
#include <iret_travel.h>	/* iret_travel */
#include <atomic_utils.h>	/* add_one_atomic */
#include <memory_manager.h> /* get_new_page_table, vm_enable_task */
#include <variable_queue.h> /* Q_INSERT_TAIL */
#include <lib_thread_management/hashmap.h>	/* map_* functions */
#include <lib_thread_management/mutex.h>	/* mutex_t */

#define ELF_IF (1 << 9);

static uint32_t get_unique_tid( void );
static uint32_t get_unique_pid( void );
static uint32_t get_user_eflags( void );

Q_NEW_HEAD(pcb_list_t, pcb);
static pcb_list_t pcb_list;


static mutex_t pcb_list_mux;
static mutex_t tcb_map_mux;

/** @brief Next pid to be assigned. Only to be updated by get_unique_pid */
static uint32_t next_pid = 0;
/** @brief Next tid to be assigned. Only to be updated by get_unique_tid */
static uint32_t next_tid = 0;

void *
get_tcb_pd(tcb_t *tcb)
{
	return tcb->owning_task->pd;
}

uint32_t
get_tcb_tid(tcb_t *tcb)
{
	return tcb->tid;
}

void
set_task_exit_status( int status )
{
	tcb_t *tcb = get_running_thread();
	affirm(tcb);
	affirm(tcb->owning_task);
	pcb_t *owning_task = tcb->owning_task;

	/* Atomically update exit status of owning task */
	mutex_lock(&owning_task->set_status_vanish_wait_mux);
	tcb->owning_task->exit_status = status;
	mutex_unlock(&owning_task->set_status_vanish_wait_mux);
}


/** @brief Initializes task manager's resources
 *
 *	 @return Void
 */
void
task_manager_init ( void )
{
	map_init();
	mutex_init(&pcb_list_mux);
	mutex_init(&tcb_map_mux);
	Q_INIT_HEAD(&pcb_list);
}

/** @brief Gets the status of a tcb
 *
 *  Needs to be guarded by synchronization from invoking function
 *
 *  @param tcb TCB from which to get status from
 *  @return Status of a thread i.e. RUNNING, RUNNABLE, DESCHEDULED, ...
 */
status_t
get_tcb_status( tcb_t *tcb )
{
	return tcb->status;
}

/** @brief Changes the page directory in the PCB to new_pd
 *
 *  @param new_pd New page directory pointer to change to
 *  @return the old page directory.
 */
void *
swap_task_pd( void *new_pd )
{
	assert(is_valid_pd(new_pd));

	/* Find PCB to swap its stored page directory */
	uint32_t pid = get_pid();
	pcb_t *pcb = find_pcb(pid);
	affirm(pcb);

	/* Swap page directories */
	void *old_pd = pcb->pd;
	pcb->pd = new_pd;

	/* Check and return the old page directory */
	affirm(is_valid_pd(old_pd));
	return old_pd;
}

/** @brief Creates a task
 *
 *	@param pid Pointer where task id for new task is stored
 *	@param tid Pointer where thread id for new thread is stored
 *	@param elf Elf header for use in allocating new task's memory
 *
 *	@return 0 on success, negative value on failure.
 * */
int
create_task( uint32_t *pid, uint32_t *tid, simple_elf_t *elf )
{
	if (!pid) return -1;
	if (!tid) return -1;
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
	return 0;
}

/** NOTE: Not to be used in context-switch, only when running task
 *	for the first time
 *
 *	Enables virtual memory of task. Use this before transplanting data
 *	into task's memory.
 *	*/
int
activate_task_memory( uint32_t pid )
{
	/* Likely messing up direct mapping of kernel memory, and
	 * some instruction after task_prepare is being seen as invalid?*/
	pcb_t *pcb;
	if ((pcb = find_pcb(pid)) == NULL)
		return -1;

	/* Update the page directory and enable VM if necessary */
	vm_enable_task(pcb->pd);

	return 0;
}

/** NOTE: Not to be used in context-switch, only when running task
 *	for the first time
 *
 *	Should only ever be called once, and after task has been initialized
 *	after a call to new_task.
 *	The caller is supposed to install memory on the new task before
 *	calling this function. Stack pointer should be appropriately set
 *	if any arguments have been loaded on stack.
 *
 *	@param tid Id of thread to run
 *	@param esp Stack pointer
 *	@param entry_point First program instruction
 *
 *	@return Never returns.
 *	*/
void
task_set_active( uint32_t tid )
{
	tcb_t *tcb;
	affirm((tcb = find_tcb(tid)) != NULL);

	/* Let scheduler know it can now run this thread */
	/* Let scheduler know it can now run this thread if it doesn't know */
	/* TODO in a bit of a pickle because we need to call disable_interrupts()
	 * to check this?
	 */
	if (tcb->status == UNINITIALIZED) {
		make_thread_runnable(tid);
	}
}

void
task_start( uint32_t tid, uint32_t esp, uint32_t entry_point )
{
	tcb_t *tcb;
	affirm((tcb = find_tcb(tid)) != NULL);

	/* Before going to user mode, update esp0, so we know where to go back to */
	set_esp0((uint32_t)tcb->kernel_stack_hi);

	/* We're currently going directly to entry point. In the future,
	 * however, we should go to some "receiver" function which appropriately
	 * sets user registers and segment selectors, and lastly RETs to
	 * the entry_point. */
	affirm(is_valid_pd((void *)TABLE_ADDRESS(get_cr3())));
	iret_travel(entry_point, SEGSEL_USER_CS, get_user_eflags(),
		esp, SEGSEL_USER_DS);

	/* NOTREACHED */
	panic("iret_travel should not return");
}

/** Looks for pcb with given pid.
 *
 *	@param pid Task id to look for
 *
 *	@return Pointer to pcb on success, NULL on failure */
pcb_t *
find_pcb( uint32_t pid )
{
	mutex_lock(&pcb_list_mux);
	pcb_t *res = Q_GET_FRONT(&pcb_list);
	while (res && res->pid != pid)
		res = Q_GET_NEXT(res, task_link);
	mutex_unlock(&pcb_list_mux);
	return res;
}

/** Looks for tcb with given tid.
 *
 *	@param tid Thread id to look for
 *
 *	@return Pointer to tcb on success, NULL on failure */
tcb_t *
find_tcb( uint32_t tid )
{
	mutex_lock(&tcb_map_mux);
	tcb_t *res = (tcb_t *)map_get(tid);
	mutex_unlock(&tcb_map_mux);
	return res;
}

/** @brief Initializes new pcb, and corresponding tcb.
 *
 *	@param pid Pointer to where pid should be stored
 *	@param pd  Pointer to page directory for new task
 *	@return 0 on success, negative value on error
 */
int
create_pcb( uint32_t *pid, void *pd )
{
	pcb_t *pcb = smalloc(sizeof(pcb_t));
	if (!pcb)
		return -1;

	//mutex_init(&thread_list_mux); TODO: Enable this
	*pid = get_unique_pid();
	pcb->pid = *pid;
	pcb->pd = pd;
	pcb->prepared = 0;

	/* Initialize thread queue */
	Q_INIT_HEAD(&(pcb->owned_threads));
	pcb->num_threads = 0;

	/* Initialize set_status_vanish_wait_mux and other structures for
	 * set_status(), vanish(), wait() */
	if (mutex_init(&(pcb->set_status_vanish_wait_mux)) < 0) {
		return -1;
	}
	Q_INIT_HEAD(&(pcb->vanished_child_tasks_list));
	Q_INIT_HEAD(&(pcb->waiting_threads_list));


	/* Add to pcb linked list*/
	mutex_lock(&pcb_list_mux);
	Q_INIT_ELEM(pcb, task_link);
	Q_INSERT_TAIL(&pcb_list, pcb, task_link);
	mutex_unlock(&pcb_list_mux);

	return 0;
}

/** @brief Initializes new tcb. Does not add thread to scheduler.
 *	   This should be done by whoever creates this thread.
 *
 *	@param pid Id of owning task
 *	@param tid Pointer to where id of new thread will be stored
 *	@return 0 on success, negative value on failure
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

    /* If debug mode is set, we let each kernel stack be PAGE_SIZE of usable
     * memory, followed by PAGE_SIZE of unusable memory to prevent kernel
     * stacks from overlapping onto each other during execution.
     */
#ifdef DEBUG
	tcb->kernel_stack_lo = smemalign(PAGE_SIZE, 2 * PAGE_SIZE);
	if (!tcb->kernel_stack_lo) {
		sfree(tcb, sizeof(tcb_t));
		return -1;
	}
	uint32_t **parent_pd = owning_task->pd;
    uint32_t pd_index = PD_INDEX(tcb->kernel_stack_lo);
	uint32_t *parent_pt = (uint32_t *) TABLE_ADDRESS(parent_pd[pd_index]);
    uint32_t pt_index = PT_INDEX(tcb->kernel_stack_lo);
	parent_pt[pt_index] = 0x0;
	tcb->kernel_stack_lo += PAGE_SIZE / sizeof(uint32_t);
#else
	tcb->kernel_stack_lo = smalloc(PAGE_SIZE);
	if (!tcb->kernel_stack_lo) {
		sfree(tcb, sizeof(tcb_t));
		return -1;
	}
#endif

	tcb->status = UNINITIALIZED;

	/* Add to owning task's list of threads */
	tcb->owning_task = owning_task;

	/* TODO: Add mutex to pcb struct and lock it here.
	 *		 For now, this just checks that we're not
	 *		 adding a second thread to an existing task. */
	affirm(!Q_GET_FRONT(&owning_task->owned_threads));

	/* Link for when this thread calls wait() */
	Q_INIT_ELEM(tcb, waiting_threads_link);

	/* Add to owning task's list of threads, increment num_threads not DEAD */
	//mutex_lock(&owning_task->thread_list_mux);
	Q_INIT_ELEM(tcb, scheduler_queue);
	Q_INIT_ELEM(tcb, tid2tcb_queue);
	Q_INIT_ELEM(tcb, owning_task_thread_list);
	Q_INSERT_TAIL(&(owning_task->owned_threads), tcb, owning_task_thread_list);
	++(owning_task->num_threads);
	//mutex_unlock(&owning_task->thread_list_mux);

	log("Inserting thread with tid %lu", tcb->tid);
	mutex_lock(&tcb_map_mux);
	map_insert(tcb);
	mutex_unlock(&tcb_map_mux);

	/* memset the whole thing, TODO delete this in future, only good for
	 * debugging when printing the whole stack
	 */
	memset(tcb->kernel_stack_lo, 0, PAGE_SIZE);

	log("create_tcb(): tcb->stack_lo:%p", tcb->kernel_stack_lo);
	// FIXME faults here

	tcb->kernel_esp = tcb->kernel_stack_lo;
	tcb->kernel_esp = (uint32_t *)(((uint32_t)tcb->kernel_esp) +
			  PAGE_SIZE - sizeof(uint32_t));
	tcb->kernel_stack_hi = tcb->kernel_esp;
	log("create_tcb(): tcb->kernel_stack_hi:%p", tcb->kernel_stack_hi);


	// set the canary
	*(tcb->kernel_stack_hi) = 0xcafebabe;

	*(tcb->kernel_stack_lo) = 0xdeadbeef;
	return 0;
	return 0;
}

/** @brief Returns number of threads in the owning task
 *
 *	The return value cannot be zero. The moment a tcb_t is initialize it
 *	is immediately added to its owning_task's owned_threads field, which is
 *	a list of threads owned by that task.
 *
 *	@param tcbp Pointer to tcb
 *	@return Number of threads in owning task
 */
int
get_num_threads_in_owning_task( tcb_t *tcbp )
{
	/* Argument checks */
	affirm_msg(tcbp, "Given tcb pointer cannot be NULL!");
	affirm_msg(tcbp->owning_task, "Tcb pointer to owning task cannot be NULL!");

	/* Check that we have a legal number of threads and return */
	uint32_t num_threads = tcbp->owning_task->num_threads;
	affirm_msg(num_threads > 0, "Owning task must have at least 1 thread!");
	return num_threads;
}

/** @brief Gets the highest writable address of the kernel stack for thread
 *	   that corresponds to supplied TCB
 *
 *	Requires that tcbp is non-NULL, and that its kernel_stack_hi field is
 *	stack aligned.
 *
 *	@param tcbp Pointer to TCB
 *	@return Kernel stack highest writable address
 */
void *
get_kern_stack_hi( tcb_t *tcbp )
{
	/* Argument checks */
	affirm_msg(tcbp, "tcbp cannot be NULL!");

	/* Invariant checks to ensure returned value is legal */
	affirm_msg(tcbp->kernel_stack_hi, "tcbp->kernel_stack_hi cannot be NULL!");
	affirm_msg(STACK_ALIGNED(tcbp->kernel_stack_hi), "tcbp->kernel_stack_hi "
		   "must be stack aligned!");
	return tcbp->kernel_stack_hi;
}

void *
get_kern_stack_lo( tcb_t *tcbp )
{
	/* Argument checks */
	affirm_msg(tcbp, "tcbp cannot be NULL!");

	/* Invariant checks to ensure returned value is legal */
	affirm_msg(tcbp->kernel_stack_lo, "tcbp->kernel_stack_lo cannot be NULL!");
	affirm_msg(STACK_ALIGNED(tcbp->kernel_stack_lo), "tcbp->kernel_stack_lo "
		   "must be stack aligned!");
	return tcbp->kernel_stack_lo;
}

/** @brief Sets the kernel_esp field in supplied TCB
 *
 *	Requires that tcbp is non-NULL and that kernel_esp is stack aligned and also
 *	non-NULL
 *
 *	@param tcbp Pointer to TCB
 *	@param kernel_esp Kernel esp the next time this thread goes into kernel
 *	   mode either through a context switch or mode switch
 *	@return Void.
 */
void
set_kern_esp( tcb_t *tcbp, uint32_t *kernel_esp )
{
	/* Argument checks */
	affirm_msg(tcbp, "tcbp cannot be NULL!");
	affirm_msg(kernel_esp, "kernel_esp cannot be NULL!");
	affirm_msg(STACK_ALIGNED(kernel_esp), "kernel_esp must be stack aligned!");

	tcbp->kernel_esp = kernel_esp;
}

/* ------ HELPER FUNCTIONS ------ */

/** @brief Returns eflags with PL altered to 3 */
static uint32_t
get_user_eflags( void )
{
	uint32_t eflags = get_eflags();

	/* Any IOPL | EFL_IOPL_RING3 == EFL_IOPL_RING3 */
	eflags |= EFL_IOPL_RING0; /* Set privilege level to user */
	eflags |= EFL_RESV1;	  /* Maitain reserved as 1 */
	eflags &= ~(EFL_AC);	  /* Disable alignment-checking */
	eflags |= EFL_IF;		  /* Enable hardware interrupts */

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

/** @brief Returns the pid of the currently running thread
 *
 *  @return Void.
 */
uint32_t
get_pid( void )
{
	/* Get TCB */
	tcb_t *tcb = get_running_thread();
	assert(tcb);

	/* Get PCB to get and return pid */
	pcb_t *pcb = tcb->owning_task;
	assert(pcb);
	uint32_t pid = pcb->pid;
	return pid;
}

/** @brief Frees a TCB
 *
 *  @pre TCB must not be in any list/queue
 *  @param tcb Pointer to TCB to be freed
 *  @return Void.
 */
void
free_tcb(tcb_t *tcb)
{
	affirm(tcb);
	affirm(!(Q_IN_SOME_QUEUE(tcb, waiting_threads_link)));
	affirm(!(Q_IN_SOME_QUEUE(tcb, scheduler_queue)));
	affirm(!(Q_IN_SOME_QUEUE(tcb, tid2tcb_queue)));
	affirm(!(Q_IN_SOME_QUEUE(tcb, owning_task_thread_list)));
	sfree(tcb, sizeof(tcb));
}



