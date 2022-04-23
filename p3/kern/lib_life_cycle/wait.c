/** @brief wait.c
 *
 *  Implements wait()
 */
#include <x86/asm.h>   /* outb() */
#include <x86/interrupt_defines.h> /* INT_CTL_PORT, INT_ACK_CURRENT */
#include <logger.h>    /* log() */
#include <timer_driver.h>		/* get_total_ticks() */
#include <scheduler.h>			/* yield_execution() */
#include <task_manager_internal.h>
#include <lib_thread_management/hashmap.h>
#include <simics.h>

static void store_waiting_thread( tcb_t *waiting_thread,
                                  void *owning_task_mux );

int
wait (int *status_ptr)
{
	tcb_t *waiting_thread = get_running_thread();
	affirm(waiting_thread);
	affirm(!(waiting_thread->collected_vanished_child));

	pcb_t *owning_task = waiting_thread->owning_task;
	affirm(owning_task);

	log_info("wait(): beginning wait "
	         "waiting_thread->tid:%d, "
			 "waiting_thread->owning_task->first_thread_tid:%d",
			 waiting_thread->tid, owning_task->first_thread_tid);

	int tid = -1;

	mutex_lock(&(owning_task->set_status_vanish_wait_mux));
	waiting_thread->collected_vanished_child =
		Q_GET_FRONT(&(owning_task->vanished_child_tasks_list));

	/* Put self on list and yield if there's possible children to collect */

	/* Could not collect vanished child on first try */
	if (!waiting_thread->collected_vanished_child) {

		if (owning_task->num_waiting_threads
			< (owning_task->num_active_child_tasks
			   + owning_task->num_vanished_child_tasks)) {

			affirm(yield_execution(BLOCKED, NULL, store_waiting_thread,
				   &(owning_task->set_status_vanish_wait_mux)) == 0);

		/* Will definitely never successfully wait for a child task */
		} else {
			mutex_unlock(&(owning_task->set_status_vanish_wait_mux));
			return -1;
		}
	/* Collected vanished child on first try, so remove them from vanish list*/
	} else {
		Q_REMOVE(&(owning_task->vanished_child_tasks_list),
		         waiting_thread->collected_vanished_child,
                 vanished_child_tasks_link);
		owning_task->num_vanished_child_tasks--;
		mutex_unlock(&(owning_task->set_status_vanish_wait_mux));
	}
	/* We've been woken up */

	affirm(waiting_thread->collected_vanished_child);

	/* Get needed information and return */
	//TODO problem. The actual tid is uint32_t
	tid = waiting_thread->collected_vanished_child->first_thread_tid;
	affirm(tid >= 0);
	if (status_ptr) {
		*status_ptr = waiting_thread->collected_vanished_child->exit_status;

	}
	if (status_ptr) {
		log_info("wait(): "
				 "waiting_thread->collected_vanished_child->first_thread_tid:%d, "
				 "exit_status:%d", tid, *status_ptr);
	} else {
		log_info("wait(): "
				 "waiting_thread->collected_vanished_child->first_thread_tid:%d, "
				 "exit_status (ignored):%d", tid, waiting_thread->collected_vanished_child->exit_status);
	}



	// TODO cleanup
	free_pcb_but_not_pd(waiting_thread->collected_vanished_child);
	waiting_thread->collected_vanished_child = NULL;
	return tid;
}

/** @brief Stores waiting thread in its task's waiting threads list and
 *         releases the tasks PCB's mutex
 */
static void
store_waiting_thread( tcb_t *waiting_thread, void *owning_task_mux )
{
	affirm(waiting_thread);
	affirm(waiting_thread->status == BLOCKED);

	pcb_t *owning_task = waiting_thread->owning_task;

	mutex_t *mux = (mutex_t *) owning_task_mux;
	affirm(mux);
	affirm(mux == &(owning_task->set_status_vanish_wait_mux));

	Q_INSERT_TAIL(&(owning_task->waiting_threads_list), waiting_thread,
				  waiting_threads_link);
	owning_task->num_waiting_threads++;


	switch_safe_mutex_unlock(mux);
	log_warn("store_waiting_thread(): "
	         "unlocked mux:%p, &waiting_threads_list:%p", mux,
			 &(owning_task->waiting_threads_list));
}

