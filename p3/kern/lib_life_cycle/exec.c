/** @file exec.c
 *  @brief Contains exec interrupt handler and helper functions for
 *         installation
 *
 *  @author Nicklaus Choo (nchoo)
 */

#include <logger.h> /* log() */
#include <assert.h> /* affirm() */
#include <task_manager.h> /* get_num_threads_in_owning_task() */
#include <scheduler.h> /* get_running_tid() */
#include <x86/interrupt_defines.h> /* INT_CTL_PORT, INT_ACK_CURRENT */
#include <x86/asm.h> /* outb() */

/** @brief Prints arguments passed to exec() when log level is DEBUG
 *
 *  @param execname Executable name
 *  @param argvec Argument vector
 *  @return Void.
 */
static void
log_exec_args( char *execname, char **argvec)
{
	log("exec name is '%s'", execname);
	int i = 0;
	while (argvec[i]) {
		log("argvec[%d]:'%s'", i, argvec[i]);
		++i;
	}
	log("argvec has %d elements", i);
}


int
exec( char *execname, char **argvec )
{
	/* Acknowledge interrupt immediately */
    outb(INT_CTL_PORT, INT_ACK_CURRENT);

	/* Only allow exec of task that has 1 thread */
	int tid = get_running_tid();
	tcb_t *parent_tcb = find_tcb(tid);
	assert(parent_tcb);
	int num_threads = get_num_threads_in_owning_task(parent_tcb);

	log("Forking task with number of threads:%ld", num_threads);
	if (num_threads > 1) {
		return -1;
	}
	assert(num_threads == 1);


	if (!execname) {
		return -1;
	}
	if (!argvec) {
		return -1;
	}
	// TODO check if argvec[0] == execname
	// TODO limit number of args
	// TODO limit length of each arg
	// TODO ensure no software exception handler registered
	log_exec_args(execname, argvec);

	/* Count number of arguments */
	int argc = 0;
	int i = 0;
	while (argvec[i]) {
		++argc;
		++i;
	}
	(void) argc;
	(void) i;



	return -1;
}

