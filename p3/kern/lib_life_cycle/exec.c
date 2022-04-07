/** @file exec.c
 *  @brief Contains exec interrupt handler and helper functions for
 *         installation
 *
 *  @author Nicklaus Choo (nchoo)
 */
#include <elf_410.h>    /* simple_elf_t, elf_load_helper */
#include <seg.h>	/* SEGSEL_... */
#include <eflags.h>	/* get_eflags*/
#include <string.h> /* strlen(), memcpy(), memset() */

#include <loader.h> /* transplant_program_memory() */
#include <page.h> /* PAGE_SIZE */
#include <memory_manager.h> /* PAGE_ALIGNED() */
#include <common_kern.h> /* USER_MEM_START */
#include <x86/cr.h> /* get_cr3() */
#include <logger.h> /* log() */
#include <assert.h> /* affirm() */
#include <task_manager.h> /* get_num_threads_in_owning_task() */
#include <iret_travel.h> /* iret_travel() */
#include <scheduler.h> /* get_running_tid() */
#include <x86/interrupt_defines.h> /* INT_CTL_PORT, INT_ACK_CURRENT */
#include <x86/asm.h> /* outb() */

#define EXEC_STR_LEN 256
#define NUM_EXEC_ARGS 16

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

/** @brief Checks that the address of every character in the string is a valid
 *         address
 *
 *  @param s String to be checked
 */
int is_valid_user_string( char *s )
{

/** @brief Executes execname with arguments in argvec
 *
 *  argvec can have a maximum of 16 elements, which is admittedly an arbitrary
 *  power of 2, but most executable invocations usually use less than 16
 *  parameters.
 *
 *  execname and each of the string arguments can have < EXEC_STR_LEN
 *  characters, which is a gain another reasonable limit for string length.
 *
 *  @param execname Executable name
 *  @param argvec String array of arguments for executable execname
 *  @return Does not return on success, -1 on error
 */
int
exec( char *execname, char **argvec )
{
	/* Acknowledge interrupt immediately */
    outb(INT_CTL_PORT, INT_ACK_CURRENT);

	/* Only allow exec of task that has 1 thread */
	uint32_t tid = get_running_tid();
	tcb_t *parent_tcb = find_tcb(tid);
	assert(parent_tcb);
	int num_threads = get_num_threads_in_owning_task(parent_tcb);

	log("Exec() task with number of threads:%ld", num_threads);
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
		if (strlen(argvec[i]) >= EXEC_STR_LEN) {
			return -1;
		}
		++argc;
		++i;
	}
	(void) argc;
	if (argc > NUM_EXEC_ARGS) {
		return -1;
	}
	/* This is on the kernel stack and is unaffected by page directory */
	char fname[EXEC_STR_LEN];
	memset(fname, 0, EXEC_STR_LEN);
	memcpy(fname, execname, strlen(execname));

	/* TODO argvec also has to be transferred to kernel memory */
	char kern_stack_args[NUM_EXEC_ARGS * EXEC_STR_LEN];
	char *kern_stack_argvec[NUM_EXEC_ARGS];

	memset(kern_stack_args, 0, NUM_EXEC_ARGS * EXEC_STR_LEN);
	memset(kern_stack_argvec, 0, NUM_EXEC_ARGS * EXEC_STR_LEN);

	int offset = 0;
	for (int i = 0; argvec[i]; ++i) {
		char *arg = argvec[i];
		memcpy(kern_stack_args + offset, arg, strlen(arg));
		kern_stack_argvec[i] = kern_stack_args + offset;
		offset += EXEC_STR_LEN;
	}

	log("fname:'%s'", fname);
	i = 0;
	while (kern_stack_argvec[i]) {
		log("kern_stack_argvec[%d]:'%s'", i, kern_stack_argvec[i]);
		++i;
	}


	if (execute_user_program(fname, argc, kern_stack_argvec, 1) < 0) {
		return -1;
	}


	return -1;
}

