/** @file exec.c
 *	@brief Contains exec interrupt handler and helper functions for
 *		   installation
 *
 *	@author Nicklaus Choo (nchoo)
 */
#include <elf_410.h>	/* simple_elf_t, elf_load_helper */
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

#define USER_STR_LEN 256
#define NUM_USER_ARGS 16

/** @brief Prints arguments passed to exec() when log level is DEBUG
 *
 *	@param execname Executable name
 *	@param argvec Argument vector
 *	@return Void.
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
 *		   address
 *
 *	The maximum permitted string length is USER_STR_LEN, including '\0'
 *	terminating character. Therefore the longest possible user string will
 *	have at most USER_STR_LEN - 1 non-NULL characters.
 *
 *	This does not check for the existence of a user executable with this
 *	name. That is done when we try to fill in the ELF header.
 *
 *	@param s String to be checked
 *	@return 1 if valid user string, 0 otherwise
 */
int
is_valid_user_string( char *s )
{
	/* Check address of every character in s */
	int i;
	for (i = 0; i < USER_STR_LEN; ++i) {

		if (!is_user_pointer_valid(s + i)) {
			log_warn("invalid address %p at index %d of user string %s",
					 s + i, i, s);
			return 0;

		} else {

			/* String has ended within USER_STR_LEN */
			if (s[i] == '\0') {
				break;
			}
		}
	}
	/* Check length of s */
	if (i == USER_STR_LEN) {
		log_warn("user string of length >= USER_STR_LEN");
		return 0;
	}
	return 1;
}

/** @brief Checks address of every char * in argvec, argvec has max length
 *		   of < NUM_USER_ARGS
 *
 *	@param execname Executable name
 *	@param argvec Argument vector
 *	@return Number of user args if valid argvec, 0 otherwise
 */
int
is_valid_user_argvec( char *execname,  char **argvec )
{
	/* Check address of every char * in argvec */
	int i;
	for (i = 0; i < NUM_USER_ARGS; ++i) {

		/* Invalid char ** */
		if (!is_user_pointer_valid(argvec + i)) {
			log_warn("invalid address %p at index %d of argvec", argvec + i, i);
			return 0;

		/* Valid char **, so check if char * is valid */
		} else {

			/* String has ended within NUM_USER_ARGS */
			if (argvec[i] == NULL) {
				break;
			}
			/* Check if valid string */
			if (!is_valid_user_string(argvec[i])) {
				log_warn("invalid address user string %s at index %d of argvec",
						 argvec[i], i);
				return 0;
			}
		}
	}
	/* Check length of arg_vec */
	if (i == NUM_USER_ARGS) {
		log_warn("argvec has length >= NUM_USER_ARGS");
		return 0;
	}
	/* Check if argvec[0] == execname */
	if (strcmp(argvec[0],execname) != 0) {
		log_warn("argvec[0]:%s not equal to execname:%s", argvec[0], execname);
		return 0;
	}
	return i;
}

/** @brief Executes execname with arguments in argvec
 *
 *	argvec can have a maximum of 16 elements, which is admittedly an arbitrary
 *	power of 2, but most executable invocations usually use less than 16
 *	parameters.
 *
 *	execname and each of the string arguments can have < USER_STR_LEN
 *	characters, which is a gain another reasonable limit for string length.
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
	uint32_t tid = get_running_tid();
	tcb_t *parent_tcb = find_tcb(tid);
	assert(parent_tcb);
	int num_threads = get_num_threads_in_owning_task(parent_tcb);

	log("Exec() task with number of threads:%ld", num_threads);
	if (num_threads > 1) {
		return -1;
	}
	assert(num_threads == 1);

	/* Validate execname */
	if (!is_valid_user_string(execname)) {
		return -1;
	}
	/* Validate argvec */
	int argc = 0;
	if (!(argc = is_valid_user_argvec(execname, argvec))) {
		return -1;
	}
	// TODO ensure no software exception handler registered
	log_exec_args(execname, argvec);

	/* Transfer execname to kernel stack so unaffected by page directory */
	char kern_stack_execname[USER_STR_LEN];
	memset(kern_stack_execname, 0, USER_STR_LEN);
	memcpy(kern_stack_execname, execname, strlen(execname));

	/* char array to store each argvec string on kernel stack */
	char kern_stack_args[NUM_USER_ARGS * USER_STR_LEN];
	memset(kern_stack_args, 0, NUM_USER_ARGS * USER_STR_LEN);

	/* char * array for argvec on kernel stack */
	char *kern_stack_argvec[NUM_USER_ARGS];
	memset(kern_stack_argvec, 0, NUM_USER_ARGS);

	/* Transfer argvec to kernel stack so unaffected by page directory */
	int offset = 0;
	for (int i = 0; argvec[i]; ++i) {
		char *arg = argvec[i];
		memcpy(kern_stack_args + offset, arg, strlen(arg));
		kern_stack_argvec[i] = kern_stack_args + offset;
		offset += USER_STR_LEN;
	}
	/* Execute */
	if (execute_user_program(kern_stack_execname, argc, kern_stack_argvec, 1)
		< 0) {
		return -1;
	}
	panic("exec() should not come here!");
	return -1;
}
