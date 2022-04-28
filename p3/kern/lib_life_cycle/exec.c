/** @file exec.c
 *	@brief Contains exec interrupt handler and helper functions
 *
 *	@author Nicklaus Choo (nchoo)
 */

#include <string.h>    /* strlen(), memcpy(), memset() */
#include <loader.h>    /* exec_user_program() */
#include <logger.h>    /* log() */
#include <assert.h>    /* affirm() */
#include <x86/asm.h>   /* outb() */
#include <scheduler.h> /* get_running_tid() */
#include <task_manager.h>   /* get_num_threads_in_owning_task() */
#include <memory_manager.h> /* is_valid_user_string(), is_valid_user_argvec() */
#include <x86/interrupt_defines.h> /* INT_CTL_PORT, INT_ACK_CURRENT */
#include <simics.h>

/* TODO: Fix abstraction */
#include <task_manager_internal.h>

/** @brief Prints arguments passed to exec() when log level is DEBUG
 *
 *	@param execname Executable name
 *	@param argvec Argument vector
 *	@return Void.
 */
static void
log_exec_args( char *execname, char **argvec )
{
	log("exec name is '%s'", execname);
	int i = 0;
	while (argvec[i]) {
		log("argvec[%d]:'%s'", i, argvec[i]);
		++i;
	}
	log("argvec has %d elements", i);
}

/** @brief Executes execname with arguments in argvec
 *
 *	argvec can have a maximum of NUM_USER_ARGS, which is admittedly an arbitrary
 *	power of 2, but most executable invocations usually use less than 16
 *	parameters.
 *
 *	execname and each of the string arguments can have < USER_STR_LEN
 *	characters, which is a gain another reasonable limit for string length.
 *
 *	NUM_USER_ARGS, USER_STR_LEN defined in memory_manager.h
 *
 *	@param execname Executable name
 *	@param argvec String array of arguments for executable execname
 *	@return Does not return on success, -1 on error
 */
int
exec( char *execname, char **argvec )
{
	/* Acknowledge interrupt immediately */
	outb(INT_CTL_PORT, INT_ACK_CURRENT);

	/* Only allow exec of task that has 1 thread */
	tcb_t *tcb = get_running_thread();

	assert(tcb);
	int num_threads = get_num_active_threads_in_owning_task(tcb);

	log("Exec() task with number of threads:%ld", num_threads);

	if (num_threads > 1) {
		return -1;
	}
	assert(num_threads == 1);

	/* Ensure no software exception handler registered */
	tcb->has_swexn_handler = 0;
	tcb->swexn_handler = 0;
	tcb->swexn_stack = 0;
	tcb->swexn_arg = NULL;

	log_warn("tcb->has_swexn_handler %d, for tcb %p", tcb->has_swexn_handler,
			tcb);

	log_exec_args(execname, argvec);

	/* Execute */
	if (execute_user_program(execname, argvec)
		< 0) {
		return -1;
	}
	panic("exec() should not come here!");
	return -1;
}
