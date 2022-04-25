/** @file vanish.c
 *  @brief Implements vanish()
 *
 *  When implementing vanish(), there are a few natural choices for deciding
 *  when to clean up resources held by a thread's TCB.
 *
 *  The first choice is when the thread calls vanish, it cleans up after
 *  itself. However, this does not work since we are unable to free the thread's
 *  kernel execution stack before it context switches away to another thread.
 *
 *  The second choice is having the next thread clean up after the vanishing
 *  thread. However, it is fundamentally impossible to guarantee that the
 *  next thread was not in the middle of executing a function from the malloc
 *  family. If it is the case where the next thread was context switched away
 *  while executing say, smemalign() for example, when we context switch from
 *  the vanishing thread back to that thread, it can't first run another
 *  malloc family function such as sfree() or free() on the vanished thread's
 *  TCB since malloc family functions are not re-entrant.
 *  Even if we guard the malloc family functions with concurrency primitive(s),
 *  the same thread which is now waiting to run sfree() or free() cannot make
 *  progress on its original malloc family function either, and has now
 *  deadlocked itself.
 *
 *  The third feasible choice then is having the last thread of a task that
 *  calls vanish() clean up after all its sibling threads belonging to the
 *  same task. Although this operation is linear in the number of sibling
 *  threads, with the bankers method of determining amortized cost, we
 *  still run in amortized constant time since freeing each vanished sibling
 *  TCB has been "paid for" when that sibling called vanish() earlier.
 */
#include <x86/asm.h>   /* outb() */
#include <x86/interrupt_defines.h> /* INT_CTL_PORT, INT_ACK_CURRENT */
#include <logger.h>    /* log() */
#include <timer_driver.h>		/* get_total_ticks() */
#include <scheduler.h>			/* yield_execution() */
#include <task_manager_internal.h>
#include <lib_thread_management/hashmap.h>
#include <memory_manager.h> /* get_initial_pd() */
#include <x86/cr.h>		/* {get,set}_{cr0,cr3} */
#include <malloc.h> /* sfree() */
#include <scheduler.h> /* make_thread_runnable() */
#include <simics.h>

void
noop (void ){
	return;
}

void assign_child_task_to_parent_thread( tcb_t *child_last_thread,
                                    void *v_waiting_thread );

void
call_back_mutex_unlock( tcb_t *unused, void *v_parent_pcb_muxp );

/** @brief Frees all TCBs in a task except the last running thread's TCB
 *
 *  @pre There are no more active threads in the task
 *
 *  @param owning_task PCB of task to free TCBs from
 *  @param last_tcb TCB of last running thread of owning_task
 *  @return Void.
 */
void
free_sibling_tcb(pcb_t *owning_task, tcb_t *last_tcb)
{
	/* Check preconditions */
	affirm(Q_GET_FRONT(&(owning_task->active_threads_list)) == NULL);
	affirm(Q_GET_TAIL(&(owning_task->active_threads_list)) == NULL);
	affirm(owning_task->num_active_threads == 0);

	/* Free TCBs of all sibling threads except for last_tcb */
	uint32_t removed = 0;
	tcb_t *curr = Q_GET_FRONT(&(owning_task->vanished_threads_list));
	while (curr && curr != last_tcb) {
		tcb_t *next = Q_GET_NEXT(curr, task_thread_link);
		Q_REMOVE(&(owning_task->vanished_threads_list), curr, task_thread_link);
		map_remove(curr->tid);
		free_tcb(curr);
		removed++;
		curr = next;
	}
	/* The last TCB must be last_tcb */
	affirm(removed == owning_task->num_vanished_threads - 1);
	affirm(Q_GET_FRONT(&(owning_task->vanished_threads_list)) == last_tcb);
	affirm(Q_GET_TAIL(&(owning_task->vanished_threads_list)) == last_tcb);
}

/** @brief Frees a PCB's page directory and uses the initial page directory.
 *
 *  @pre No threads in PCB must intend to run again
 *
 *  @param owning_task PCB pointer to free page directory from
 *  @return Void.
 */
void
free_task_pd( pcb_t *owning_task )
{
	affirm(owning_task);

	/* Swap out page directory to the initial page directory */
	uint32_t **initial_pd = get_initial_pd();
	affirm(initial_pd);
	uint32_t **current_pd = (uint32_t **) TABLE_ADDRESS(get_cr3());
	affirm(PAGE_ALIGNED(current_pd));
	affirm(PAGE_ALIGNED(owning_task->pd));
	affirm(PAGE_ALIGNED(initial_pd));
	affirm(current_pd == owning_task->pd);
	set_cr3((uint32_t) initial_pd);

	/* Free task page directory */
	free_pd_memory(owning_task->pd);
	sfree(owning_task->pd, PAGE_SIZE);

	/* Set owning_task->pd to NULL to prevent future free-ing */
	owning_task->pd = NULL;
}

/** @brief Empty list of active child tasks
 *
 *  @param owning_task PCB from which to empty list of active child tasks
 *  @return Void.
 */
void
empty_active_child_tasks_list( pcb_t *owning_task )
{
	pcb_t *active_child = Q_GET_FRONT(&(owning_task->active_child_tasks_list));

	while (active_child) {
		Q_REMOVE(&(owning_task->active_child_tasks_list), active_child,
				 vanished_child_tasks_link);
		owning_task->num_active_child_tasks--;
		active_child = Q_GET_FRONT(&(owning_task->active_child_tasks_list));
	}
}




/** @brief Makes a task ready for collection by its parent, ceases task
 *         thread execution that calls vanish.
 *
 *  //TODO do a version that takes in an argument on error, or does
 *  panic_thread do that?
 */
void
_vanish( void )
{
	/* Get TCB and PCB and obtain critical section */
	tcb_t *tcb = get_running_thread();
	pcb_t *owning_task = tcb->owning_task;
	mutex_lock(&(owning_task->set_status_vanish_wait_mux));

	/* Move current TCB from active threads to vanished threads */
	Q_REMOVE(&(owning_task->active_threads_list), tcb, task_thread_link);
	(owning_task->num_active_threads)--;

	Q_INSERT_TAIL(&(owning_task->vanished_threads_list), tcb, task_thread_link);
	(owning_task->num_vanished_threads)++;

	affirm(owning_task->num_active_threads + owning_task->num_vanished_threads
	       == owning_task->total_threads);

	/* Not the last task, yield elsewhere */
	if (get_num_active_threads_in_owning_task(tcb) > 0) {
		log("_vanish(): not last task thread");
		mutex_unlock(&(owning_task->set_status_vanish_wait_mux));
		affirm(yield_execution(DEAD, NULL, NULL, NULL) == 0);

	/* Last task thread cleans up and contacts parent/init PCB */
	} else {
		log("_vanish(): last task thread");
		remove_pcb(owning_task);

		/* Free sibling threads TCB */
		owning_task->last_thread = tcb;
		free_sibling_tcb(owning_task, tcb);
		mutex_unlock(&(owning_task->set_status_vanish_wait_mux));

		/* Free page directory */
		free_task_pd(owning_task);

		/* All my active child tasks will automatically look for init,
		 * time to clear my active child tasks list */
		empty_active_child_tasks_list(owning_task);

		/* Transfer all my vanished children to init in O(1) */
		pcb_t *init_pcbp = get_init_pcbp();
		if (Q_GET_FRONT(&(owning_task->vanished_child_tasks_list))) {

			mutex_lock(&(init_pcbp->set_status_vanish_wait_mux));

			Q_APPEND(&(init_pcbp->vanished_child_tasks_list),
					 &(owning_task->vanished_child_tasks_list),
					 vanished_child_tasks_link);
			init_pcbp->num_vanished_child_tasks
				+= owning_task->num_vanished_child_tasks;

			log("_vanish(): added my children to init_pcbp");

			/* wakeup if there's a waiting init thread */
			tcb_t *waiting_tcb =
			    Q_GET_FRONT(&(init_pcbp->waiting_threads_list));
			if (waiting_tcb) {
				pcb_t *child =
					Q_GET_FRONT(&(init_pcbp->vanished_child_tasks_list));

				Q_REMOVE(&(init_pcbp->vanished_child_tasks_list), child,
						 vanished_child_tasks_link);
				init_pcbp->num_vanished_child_tasks--;

				assign_child_task_to_parent_thread(child->last_thread,
					waiting_tcb);
			} else {
				mutex_unlock(&(init_pcbp->set_status_vanish_wait_mux));
			}
		}

		/* Insert into parent PCB's list of vanished child tasks */
		pcb_t *parent_pcb = find_pcb(owning_task->parent_pid);

		if (parent_pcb) {

			mutex_lock(&(parent_pcb->set_status_vanish_wait_mux));
			log("_vanish(): found my parent ");
			noop();

			/* Remove from active_child_tasks_list */
			Q_REMOVE(&parent_pcb->active_child_tasks_list, owning_task,
					 vanished_child_tasks_link);
			parent_pcb->num_active_child_tasks--;

		} else {
			parent_pcb = init_pcbp;
			assert(parent_pcb);
			mutex_lock(&(parent_pcb->set_status_vanish_wait_mux));
			log("(init) parent_pcb->execname:%s", parent_pcb->execname);
			noop();

		}
		affirm(parent_pcb);

		/* Look at list of waiting parent threads, if non-empty, wake up */
		tcb_t *waiting_tcb = Q_GET_FRONT(&(parent_pcb->waiting_threads_list));
		if (waiting_tcb) {

			log("_vanish(): "
			    "collected owning_task->first_thread_tid:%d, exit_status:%d",
				owning_task->first_thread_tid, owning_task->exit_status);

			/* Yield to parent task's waiting thread */
			affirm(yield_execution(DEAD, NULL,
		           assign_child_task_to_parent_thread, waiting_tcb) == 0);

		/* No parent threads waiting, add self to vanished child list */
		} else {
			Q_INSERT_TAIL(&(parent_pcb->vanished_child_tasks_list),
						  owning_task, vanished_child_tasks_link);
			parent_pcb->num_vanished_child_tasks++;
			log("_vanish(): no parent waiting for me");

			affirm(yield_execution(DEAD, NULL, call_back_mutex_unlock,
				&(parent_pcb->set_status_vanish_wait_mux)) == 0);
		}

	}
}

void
call_back_mutex_unlock( tcb_t *unused, void *v_parent_pcb_muxp )
{
	assert(unused);
	assert(v_parent_pcb_muxp);
	mutex_t *parent_pcb_muxp = (mutex_t *) v_parent_pcb_muxp;
	switch_safe_mutex_unlock(parent_pcb_muxp);
}


/** @brief Assigns the first waiting parent thread a vanished child task
 *
 *  @pre There must be a waiting parent thread
 *  @pre There must be a vanished child task PCB
 *
 *  @param parent_pcb PCB of task with waiting parent threads.
 */
void
assign_child_task_to_parent_thread( tcb_t *child_last_thread,
                                    void *v_waiting_thread )
{
	affirm(child_last_thread);
	affirm(child_last_thread->status == DEAD);

	affirm(v_waiting_thread);
	tcb_t *waiting_thread = (tcb_t *) v_waiting_thread;

	pcb_t *child_pcb = child_last_thread->owning_task;
	affirm(child_pcb);

	pcb_t *parent_pcb = waiting_thread->owning_task;
	affirm(parent_pcb);

	affirm(waiting_thread == Q_GET_FRONT(&(parent_pcb->waiting_threads_list)));

	Q_REMOVE(&(parent_pcb->waiting_threads_list), waiting_thread,
	         waiting_threads_link);
	parent_pcb->num_waiting_threads--;

	/* Waiting thread must not be on the scheduler queue or any other
	 * queues that use the same link name eg mutex queue */
	affirm_msg(!Q_IN_SOME_QUEUE(waiting_thread, scheduler_queue),
	           "waiting_tcb:%p in scheduler queue!", waiting_thread);
	affirm(waiting_thread->status == BLOCKED);

	waiting_thread->collected_vanished_child = child_pcb;

	switch_safe_mutex_unlock(&(parent_pcb->set_status_vanish_wait_mux));
	switch_safe_make_thread_runnable(waiting_thread);
}



void
vanish( void )
{
	/* Acknowledge interrupt immediately */
	outb(INT_CTL_PORT, INT_ACK_CURRENT);
	log_info("call vanish");
	_vanish();
}
