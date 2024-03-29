/** @file kernel.c
 *  @brief The entrypoint into the kernel
 *
 *  Kernel initialization happens here
 *
 *  @author Harry Q. Bovik (hqbovik)
 *  @author Fred Hacker (fhacker)
 *  @author Nicklaus Choo (nchoo)
 *  @author Andre Nascimento (anascime)
 *  @bug No known bugs.
 */

#include <logger.h> /* log(), log_med(), log_hi() */
#include <install_handler.h> /* handler_install() */
#include <common_kern.h>

/* libc includes. */
#include <stdio.h>

/* multiboot header file */
#include <multiboot.h>              /* boot_info */

/* x86 specific includes */
#include <cr.h>		/* get/set_cr0() */
#include <asm.h>	/* enable_interrupts() */

#include <exec2obj.h> /* MAX_EXECNAME_LEN */

#include <logger.h>			/* log_info() */
#include <limits.h>			/* UINT_MAX */
#include <loader.h>			/* execute_user_program() */
#include <console.h>    	/* init_console() */
#include <scheduler.h>  	/* scheduler_on_tick() */
#include <task_manager.h>	/* task_manager_init() */
#include <memory_manager.h>	/* initialize_zero_frame() */
#include <keybd_driver.h>	/* readline() */
#include <lib_thread_management/sleep.h>	/* sleep_on_tick() */
#include <simics.h>

volatile static int __kernel_all_done = 0;

#define PROTECTION_ENABLE_FLAG (1 << 0)

/* Level of logging, set to 4 to turn logging off,
 * 1 to print logs for log priorities lo, med, hi
 * 2 to print logs for log priorities med, hi
 * 3 to print logs for log priorities hi
 *
 * defining the NDEBUG flag will also turn logging off
 */
int log_level = 3;

/** @brief Callback to handle ticks. This function is responsible
 *		   for calling any other callbacks expecting ticks.
 *
 *	@param numTicks Total number of ticks since startup */
void
tick( unsigned int numTicks ) {
	/* At our tickrate of 1000Hz, after around 48 days numTicks will overflow
	 * and break a lot of things. Let the user know they should be more polite
	 * and restart their computer every other month! */
	if (numTicks == UINT_MAX) {
		panic("System has been running for too long. Please reboot every "
				"other month!");
	}

	/* The amount of work done before scheduler tick handler should be
	 * small, as it will consume time from the thread being context switched
	 * to (in most cases). */
	sleep_on_tick(numTicks);

	/* Validate stack canaries for currently running thread */
	if (get_running_thread()) {
		affirm (*((uint32_t *) get_kern_stack_hi((get_running_thread())))
		== 0xcafebabe);

		affirm(*((uint32_t *) get_kern_stack_lo((get_running_thread())))
		== 0xdeadbeef);
	}

	/* Scheduler tick handler should be last, as it triggers context_switch */
	scheduler_on_tick(numTicks);

	if (get_running_thread()) {
		affirm (*((uint32_t *) get_kern_stack_hi((get_running_thread())))
		== 0xcafebabe);

		affirm(*((uint32_t *) get_kern_stack_lo((get_running_thread())))
		== 0xdeadbeef);
	}
}

/** @brief Kernel entrypoint.
 *
 *  @param mbinfo Unused
 *  @param arg
 *  @return Does not return
 */
int
kernel_main( mbinfo_t *mbinfo, int argc, char **argv, char **envp )
{
    (void)mbinfo;
	(void)argc;
	(void)argv;
    (void)envp;

	/* initialize handlers and enable interrupts */
	if (handler_install(tick) < 0) {
		panic("cannot install handlers");
	}

	init_console();

	task_manager_init();

	init_memory_manager();

	log("this is DEBUG");
	log_info("this is INFO");
	log_warn("this is WARN");

	char *init_args[] = {"init", 0};
	affirm(load_initial_user_program("init", 1, init_args) == 0);

	char *idle_args[] = {"idle", 0};
	affirm(load_initial_user_program("idle", 1, idle_args) == 0);

	start_first_running_thread();

	/* NOTREACHED */
	panic("Kernel_main should not return");

    return 0;
}
