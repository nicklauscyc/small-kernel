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

int
wait (int *status_ptr)
{
	// TODO delete me, here only for debugging
	if (status_ptr) {
		*status_ptr = -69;
	}
	return 42;

	tcb_t *waiting_thread = get_running_thread();
	affirm(waiting_thread);
	pcb_t *owning_task = waiting_thread->owning_task;
	affirm(owning_task);

	log_info("wait(): "
	         "waiting_thread->tid:%d, owning_task->first_thread_tid:%d",
			 waiting_thread->tid, owning_task->first_thread_tid);


	int tid;

	mutex_lock(&(owning_task->set_status_vanish_wait_mux));
	pcb_t *vanished_child =
		Q_GET_FRONT(&(owning_task->vanished_child_tasks_list));

	/* Put self on list and yield if there's possible children to collect */
	MAGIC_BREAK;
	if (!vanished_child) {

		if (owning_task->num_waiting_threads
			< (owning_task->num_active_child_tasks
			   + owning_task->num_vanished_child_tasks)) {
			Q_INSERT_TAIL(&(owning_task->waiting_threads_list), waiting_thread,
			              waiting_threads_link);
			owning_task->num_waiting_threads++;

			mutex_unlock(&(owning_task->set_status_vanish_wait_mux));

			affirm(yield_execution(BLOCKED, NULL, NULL, NULL) == 0);

		/* Will definitely never successfully wait for a child task */
		} else {

			mutex_unlock(&(owning_task->set_status_vanish_wait_mux));
			return -1;
		}
		/* We've been woken up */
		// TODO should consider using a cvar here so we don't need to reacquire
		// the lock
		mutex_lock(&(owning_task->set_status_vanish_wait_mux));
	}
	/* Now we definitely can get a vanished child */
	vanished_child = Q_GET_FRONT(&(owning_task->vanished_child_tasks_list));
	affirm(vanished_child);

	/* Remove from list and relinquish lock */
	Q_REMOVE(&(owning_task->vanished_child_tasks_list), vanished_child,
			 vanished_child_tasks_link);
	owning_task->num_vanished_child_tasks--;

	mutex_unlock(&(owning_task->set_status_vanish_wait_mux));

	/* Get needed information and return */
	tid = vanished_child->first_thread_tid;
	if (status_ptr) {
		*status_ptr = vanished_child->exit_status;
	}
	// TODO cleanup
	free_pcb_but_not_pd(vanished_child);
	return tid;
}

