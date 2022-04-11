/** @file kernel.c
 *  @brief An initial kernel.c
 *
 *  You should initialize things in kernel_main(),
 *  and then run stuff.
 *
 *  @author Harry Q. Bovik (hqbovik)
 *  @author Fred Hacker (fhacker)
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
#include <keybd_driver.h>	/* readline() */
#include <lib_thread_management/sleep.h>	/* sleep_on_tick() */

volatile static int __kernel_all_done = 0;

#define PROTECTION_ENABLE_FLAG (1 << 0)

/* Level of logging, set to 4 to turn logging off,
 * 1 to print logs for log priorities lo, med, hi
 * 2 to print logs for log priorities med, hi
 * 3 to print logs for log priorities hi
 *
 * defining the NDEBUG flag will also turn logging off
 */
int log_level = 2;

void tick(unsigned int numTicks) {
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

	/* Scheduler tick handler should be last, as it triggers context_switch */
	scheduler_on_tick(numTicks);
}

void hard_code_test( char *s )
{
	char *user_argv = (char *)s;
	execute_user_program(s, 1, &user_argv);
}

/** @brief Kernel entrypoint.
 *
 *  This is the entrypoint for the kernel.
 *
 * @return Does not return
 */
int
kernel_main( mbinfo_t *mbinfo, int argc, char **argv, char **envp )
{
    /* FIXME: What to do with mbinfo and envp? */
    (void)mbinfo;
    (void)envp;

	/* initialize handlers and enable interrupts */
	if (handler_install(tick) < 0) {
		panic("cannot install handlers");
	}
	enable_interrupts();

	init_console();

	task_manager_init();
	//test_physalloc(); // TODO put in test suite


    //TODO: Run shell once exec and fork are working

	log("this is DEBUG");
	log_info("this is INFO");
	log_warn("this is WARN");

    while (!__kernel_all_done) {
		// Used for development to run a certain test straightaway
		//hard_code_test("exec_nonexist");

        int n = MAX_EXECNAME_LEN;
        char s[n];

        /* Display prompt */
        printf("pebbles>");
        int res = readline(s, n);

        if (res == n)
            continue; /* Executable name too large */

        /* Swap \n returned by readline for null-terminator */
        s[res - 1] = '\0';

        char *user_argv = (char *)s;

        execute_user_program(s, 1, &user_argv);


    }

    return 0;
}
