/** @file task_manager.c
 *  @brief Module for management of tasks.
 *
 */

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
#include <lib_memory_management/memory_management.h> /* new_pages */

#define ELF_IF (1 << 9);


static uint32_t get_unique_tid( void );
static uint32_t get_unique_pid( void );

Q_NEW_HEAD(pcb_list_t, pcb);
static pcb_list_t pcb_list;

/* List of PCBs whose running task is init() */
Q_NEW_HEAD(init_pcb_list_t, pcb);
static init_pcb_list_t init_pcb_list;
static mutex_t init_pcb_list_mux;

static mutex_t pcb_list_mux;
static mutex_t tcb_map_mux;

/** @brief Next tid to be assigned. Only to be updated by get_unique_tid
 *
 *  This starts from 1 since an uninitialized tid is 0, so we know that
 *  for any pcb such that pcb->first_thread_tid = 0 we know that within
 *  a particular call to create_tcb() we are creating the first thread
 *  of a task
 */
static uint32_t next_tid = 1;

/** @brief Next pid to be assigned. Only to be updated by get_unique_pid */
static uint32_t next_pid = 1;


/** @brief Gets the pd for a TCB pointer
 *
 *  @return pd of TCB pointer. Once freed, the pd field of owning_task will be
 *  NULL so it is ok to ruturn NULL.
 */
void *
get_tcb_pd(tcb_t *tcb)
{
	affirm(tcb);
	affirm(tcb->owning_task);
	return tcb->owning_task->pd;
}

/** @brief Returns the TCB's tid
 *
 *  @return TID of TCB
 */
uint32_t
get_tcb_tid(tcb_t *tcb)
{
	affirm(tcb);
	return tcb->tid;
}

/** @brief Sets the exit status of a task
 *
 *  @param status Status to set the task to
 */
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
	mutex_init(&init_pcb_list_mux);
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
swap_task_pd( void *new_pd, pcb_t *pcb )
{
	affirm(new_pd);
	/* Expensive consistency check is an assert not affirm */
	assert(is_valid_pd(new_pd));

	affirm(pcb);

	/* Swap page directories */
	void *old_pd = pcb->pd;
	pcb->pd = new_pd;

	/* Check and return the old page directory */
	affirm(old_pd);
	assert(is_valid_pd(old_pd));
	return old_pd;
}

/** @brief Creates a task
 *
 *	@param pid Pointer where task id for new task is stored
 *	@param tid Pointer where thread id for new thread is stored
 *	@param elf Pointer to Elf header for use in allocating new task's memory
 *	@return 0 on success, negative value on failure.
 * */
int
create_task( uint32_t *pid, uint32_t *tid, simple_elf_t *elf )
{
	affirm(pid);
	affirm(tid);
	affirm(elf);

	/* Allocates physical memory to a new page table and enables VM */
	/* Ensure alignment of page table directory */

	void *pd = new_pd_from_elf(elf);
	if (!pd) {
		return -1;
	}

	pcb_t *owning_task = create_pcb(pid, pd, NULL);
	if (!owning_task) {
		free_pd_memory(pd);
		sfree(pd, PAGE_SIZE);
		return -1;
	}

	tcb_t *new_thread = create_tcb(owning_task, tid);
	if (!new_thread) {
		free_pd_memory(pd);
		owning_task->pd = NULL;
		free_pcb_but_not_pd_no_last_thread(owning_task);
		sfree(pd, PAGE_SIZE);
		return -1;
	}
	return 0;
}

/** @brief Updates page directory to given PCB's page directory
 *
 *  NOTE: Not to be used in context-switch, only when running task
 *	for the first time
 *
 *	Enables virtual memory of task. Use this before transplanting data
 *	into task's memory.
 *
 *	@param pcb PCB pointer for which we want to use their page directory
 *	@return Void.
 */
void
activate_task_memory(pcb_t *pcb)
{
	affirm(pcb);
	affirm(pcb->pd);

	/* Update the page directory and enable VM if necessary */
	vm_enable_task(pcb->pd);
}


/** @brief Mode switches from kernel mode to user mode to start a task for the
 *         very first time.
 *
 *  @param tid Task first thread ID
 *  @param user_esp User's esp to start running task at
 *  @param entry_point First instruction that the user will run
 *  @return Does not return.
 */
void
task_start( uint32_t tid, uint32_t user_esp, uint32_t entry_point )
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
		user_esp, SEGSEL_USER_DS);

	/* NOTREACHED */
	panic("iret_travel should not return");
}

/** @brief Looks for pcb with given pid.
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

/** @brief Removes a PCB from system wide list of PCBs.
 *  @param pcbp PCB pointer
 *  @return Void.
 */
void
remove_pcb( pcb_t *pcbp )
{
	affirm(pcbp);
	mutex_lock(&pcb_list_mux);
	Q_REMOVE(&pcb_list, pcbp, task_link);
	mutex_unlock(&pcb_list_mux);
}

/** @brief Looks for tcb with given tid.
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
 *	@param parent_pcb Parent task's PCB
 *	@return Pointer to new PCB on success, NULL on error
 */
pcb_t *
create_pcb( uint32_t *pid, void *pd, pcb_t *parent_pcb)
{
	if (!pid)
		return NULL;

	if (!pd)
		return NULL;

	pcb_t *pcb = smalloc(sizeof(pcb_t));
	if (!pcb) {
		return NULL;
	}
	if (mutex_init(&(pcb->set_status_vanish_wait_mux)) < 0) {
		free_pcb_but_not_pd(pcb);
		return NULL;
	}
	pcb->pd = pd;
	*pid = get_unique_pid();
	pcb->pid = *pid;
	pcb->exit_status = 0;

	/* Immediate child tasks that have all their threads vanished */
	Q_INIT_HEAD(&(pcb->vanished_child_tasks_list));
	pcb->num_vanished_child_tasks = 0;

	/* Immediate child tasks that have at least 1 active thread */
	Q_INIT_HEAD(&(pcb->active_child_tasks_list));
	pcb->num_active_child_tasks = 0;

	/* List of task threads waiting for child threads to vanish */
	Q_INIT_HEAD(&(pcb->waiting_threads_list));
	pcb->num_waiting_threads = 0;

	/* Set parent task PCB pointer if present */
	if (parent_pcb) {
		pcb->parent_pcb = parent_pcb;
		pcb->parent_pid = parent_pcb->pid;
	} else {
		pcb->parent_pcb = NULL;
		pcb->parent_pid = 0;
	}
	/* Link to later put this PCB on its parent's vanished_child_tasks_list */
	Q_INIT_ELEM(pcb, vanished_child_tasks_link);
	pcb->total_threads = 0;

	/* For putting into list of init task PCBs */
	Q_INIT_ELEM(pcb, init_pcb_link);

	/* Initialize task's active and vanished thread lists */
	Q_INIT_HEAD(&(pcb->active_threads_list));
	pcb->num_active_threads = 0;
	Q_INIT_HEAD(&(pcb->vanished_threads_list));
	pcb->num_vanished_threads = 0;
	pcb->first_thread_tid = 0;
	pcb->last_thread = NULL;

	/* Add to pcb linked list */
	mutex_lock(&pcb_list_mux);
	Q_INIT_ELEM(pcb, task_link);
	Q_INSERT_TAIL(&pcb_list, pcb, task_link);
	mutex_unlock(&pcb_list_mux);

	return pcb;
}

/** @brief Initializes new tcb. Does not add thread to scheduler.
 *	       This should be done by whoever creates this thread.
 *
 *	@param pid Id of owning task
 *	@param tid Pointer to where id of new thread will be stored
 *	@return Pointer to new TCB on success, NULL on error.
 */
tcb_t *
create_tcb( pcb_t *owning_task, uint32_t *tid )
{
	if (!owning_task)
		return NULL;

	if (!tid)
		return NULL;

	tcb_t *tcb = smalloc(sizeof(tcb_t));
	if (!tcb) {
		log_warn("create_tcb(): smalloc(sizeof(tcb_t)) returned NULL");
		return NULL;
	}

	*tid = get_unique_tid();
	tcb->tid = *tid;

	tcb->status = UNINITIALIZED;
	tcb->owning_task = owning_task;

	tcb->collected_vanished_child = NULL;

	tcb->kernel_stack_lo = smalloc(KERNEL_THREAD_STACK_SIZE);

	if (!tcb->kernel_stack_lo) {
		sfree(tcb, sizeof(tcb_t));
		log_info("create_tcb(): smalloc() kernel stack returned NULL");

		return NULL;
	}

	/* Link for when this thread calls wait() */
	Q_INIT_ELEM(tcb, waiting_threads_link);

	/* Add to owning task's list of threads, increment num_active_threads not
	 * DEAD */
	Q_INIT_ELEM(tcb, scheduler_queue);
	Q_INIT_ELEM(tcb, tid2tcb_queue);
	Q_INIT_ELEM(tcb, task_thread_link);

	mutex_lock(&owning_task->set_status_vanish_wait_mux);
	Q_INSERT_TAIL(&(owning_task->active_threads_list), tcb, task_thread_link);
	++(owning_task->num_active_threads);
	++(owning_task->total_threads);

	/* Set first thread tid of pcb */
	if (owning_task->first_thread_tid == 0) {
		owning_task->first_thread_tid = *tid;
	}
	mutex_unlock(&owning_task->set_status_vanish_wait_mux);

	log("Inserting thread with tid %lu", tcb->tid);
	mutex_lock(&tcb_map_mux);
	map_insert(tcb);
	mutex_unlock(&tcb_map_mux);

	memset(tcb->kernel_stack_lo, 0, KERNEL_THREAD_STACK_SIZE);

	log("create_tcb(): tcb->stack_lo:%p", tcb->kernel_stack_lo);
	tcb->kernel_esp = tcb->kernel_stack_lo;
	tcb->kernel_esp = (uint32_t *)(((uint32_t)tcb->kernel_esp) +
			  KERNEL_THREAD_STACK_SIZE - sizeof(uint32_t));
	tcb->kernel_stack_hi = tcb->kernel_esp;
	log("create_tcb(): tcb->kernel_stack_hi:%p", tcb->kernel_stack_hi);

	/* Set swexn info */
	tcb->has_swexn_handler = 0;
	tcb->swexn_handler = 0;
	tcb->swexn_stack = 0;
	tcb->swexn_arg = NULL;

	*(tcb->kernel_stack_hi) = 0xcafebabe;
	*(tcb->kernel_stack_lo) = 0xdeadbeef;

	return tcb;
}

/** @brief Returns number of threads in the owning task
 *
 *	The return value cannot be zero. The moment a tcb_t is initialize it
 *	is immediately added to its owning_task's active_threads field, which is
 *	a list of threads owned by that task.
 *
 *	@param tcbp Pointer to tcb
 *	@return Number of threads in owning task
 */
int
get_num_active_threads_in_owning_task( tcb_t *tcbp )
{
	/* Argument checks */
	affirm_msg(tcbp, "Given tcb pointer cannot be NULL!");
	affirm_msg(tcbp->owning_task, "Tcb pointer to owning task cannot be NULL!");

	/* Check that we have a legal number of threads and return */
	uint32_t num_active_threads = tcbp->owning_task->num_active_threads;
	affirm_msg(num_active_threads >= 0,
	           "Owning task must have non-negative threads!");
	return num_active_threads;
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

/** @brief Gets the kernel stack lowest address
 *
 *  @return Kernel stack lowest address
 */
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

/** @brief Returns eflags
 *
 *  @return Eflags for the user
 */
uint32_t
get_user_eflags( void )
{
	uint32_t eflags = get_eflags();

	eflags |= EFL_IOPL_RING0; /* Set privilege level to user */
	eflags |= EFL_RESV1;	  /* Maitain reserved as 1 */
	eflags &= ~(EFL_AC);	  /* Disable alignment-checking */
	eflags |= EFL_IF;		  /* Enable hardware interrupts */

	return eflags;
}

/** @brief Returns a unique pid.
 *
 *  @return Unique pid
 */
static uint32_t
get_unique_pid( void )
{
	return add_one_atomic(&next_pid);
}

/** @brief Returns a unique tid.
 *
 *  @return Unique tid
 */
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

/** @brief Frees a TCB and removes the TCB from tid2tcb hashmap list
 *
 *
 *  @pre TCB must not be in any list/queue except for the tid2tcb hashmap list
 *  @pre TCB must not be holding on to any vanished child taskss
 *  @pre TCB must be DEAD
 *
 *  @param tcb Pointer to TCB to be freed
 *  @return Void.
 */
void
free_tcb(tcb_t *tcb)
{
	log_warn("free_tcb(): cleaning up thread tid:%d", tcb->tid);

	affirm(tcb);
	affirm(!tcb->collected_vanished_child);
	affirm(tcb->status == DEAD);
	affirm(!(Q_IN_SOME_QUEUE(tcb, waiting_threads_link)));
	affirm(!(Q_IN_SOME_QUEUE(tcb, scheduler_queue)));
	affirm(!(Q_IN_SOME_QUEUE(tcb, tid2tcb_queue)));
	affirm(!(Q_IN_SOME_QUEUE(tcb, task_thread_link)));
	affirm(tcb->status == DEAD);


	/* free stack and structure memory */
	sfree(tcb->kernel_stack_lo, KERNEL_THREAD_STACK_SIZE);
	sfree(tcb, sizeof(tcb_t));

	log_info("free_tcb(): cleaned up thread tid:%d", tcb->tid);
}

/** @brief Frees a PCB along with selected fields
 *
 *  @param pcb PCB to be freed
 *  @param free_last_thread Whether to free the last thread field or not
 *  @return Void.
 */
void
free_pcb_but_not_pd_helper(pcb_t *pcb, int free_last_thread )
{
	affirm(pcb);

	/* pd should already be freed and set to NULL */
	affirm_msg(!pcb->pd, "pcb->pd should be null, but pcb->pd:%p", pcb->pd);
	log_warn("free_pcb_but_not_pd(): cleaned up pcb->first_thread_tid:%d",
			 pcb->first_thread_tid);


	/* All lists should be empty */
	affirm(!Q_GET_FRONT(&pcb->vanished_child_tasks_list));

	affirm(!Q_GET_FRONT(&pcb->active_child_tasks_list));

	affirm(!Q_GET_FRONT(&pcb->waiting_threads_list));


	/* Already removed from vanished_child_tasks_list */
	affirm(!Q_IN_SOME_QUEUE(pcb, vanished_child_tasks_link));
	affirm(!Q_GET_FRONT(&pcb->active_threads_list));

	/* Left to free the last task thread */
	if (free_last_thread) {
		affirm(Q_GET_FRONT(&pcb->vanished_threads_list) == pcb->last_thread);
		affirm(Q_GET_TAIL(&pcb->vanished_threads_list) == pcb->last_thread);
		affirm(pcb->last_thread);
		map_remove(pcb->last_thread->tid);
		free_tcb(pcb->last_thread);
	}

	sfree(pcb, sizeof(pcb_t));
	log_info("free_pcb_but_not_pd(): "
		     "complete cleaned up pcb->first_thread_tid:%d",
			 pcb->first_thread_tid);

}

/** @brief Frees PCB along with its last thread's TCB. Used during a call to
 *         vanish.
 *
 *  @param pcb PCB to be freed.
 *  @return Void.
 */
void
free_pcb_but_not_pd( pcb_t *pcb )
{
	affirm(pcb);
	free_pcb_but_not_pd_helper(pcb, 1);
}

/** @brief Frees a PCB but not its associated pd nor its last thread field. This
 *         is called when a TCB is failed to be allocated, so we free the
 *         PCB that was created first.
 *
 *  @param pcb PCB to be freed
 *  @return Void.
 */
void
free_pcb_but_not_pd_no_last_thread( pcb_t *pcb )
{
	affirm(pcb);
	free_pcb_but_not_pd_helper(pcb, 0);
}

/** @brief Checks if task indicated by given pid is running the 'init' task.
 *
 * 	The latest fork() or exec() which is 'init' will be the init task,
 * 	since 'init' can fork a child, and the child will run init while
 * 	the parent runs the shell
 *
 *  @pre pid must be pid of PCB running kern_stack_execname
 *  @pre kern_stack_execname must be validated
 */
void
register_if_init_task( char *execname, uint32_t pid )
{
	affirm(execname);

	mutex_lock(&init_pcb_list_mux);
	int init_strlen = strlen("init");
	if ((execname[init_strlen] == '\0')
		&& (strncmp(execname, "init", init_strlen) == 0)) {

		pcb_t *pcb = find_pcb(pid);
		affirm(pcb);

		Q_INSERT_FRONT(&init_pcb_list, pcb, init_pcb_link);
	}
	/* Remove any non init pcb */
	pcb_t *curr = Q_GET_FRONT(&init_pcb_list);
	while (curr) {
		pcb_t *next = Q_GET_NEXT(curr, init_pcb_link);
		if (safe_strcmp(curr->execname, "init") != 0) {
			Q_REMOVE(&init_pcb_list, curr, init_pcb_link);
		}
		curr = next;
	}
	mutex_unlock(&init_pcb_list_mux);

}

/** @brief Gets the PCB for the init task
 *
 * 	@pre Should only be run after loading the init task
 *  @return Returns a non-NULL pointer for the init pcb
 */
pcb_t *
get_init_pcbp( void )
{
	mutex_lock(&init_pcb_list_mux);

	pcb_t * init_pcbp = Q_GET_FRONT(&init_pcb_list);
	affirm(init_pcbp);
	affirm(safe_strcmp(init_pcbp->execname, "init") == 0);

	mutex_unlock(&init_pcb_list_mux);

	return init_pcbp;
}


/** @brief sets the task name
 *
 *  @pre execname must be a valid non-malicious string
 *  @return Void.
 */
void
set_task_name( pcb_t *pcbp, char *execname )
{
	affirm(pcbp);
	affirm(execname);

	memset(pcbp->execname, 0, USER_STR_LEN);
	memcpy(pcbp->execname, execname, strlen(execname));
}
