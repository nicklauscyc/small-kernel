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
#include <x86/asm.h>                /* enable_interrupts() */

#include <x86/cr.h> /* get_cr3() */

#include <exec2obj.h> /* MAX_EXECNAME_LEN */

#include <loader.h>     /* execute_user_program() */
#include <console.h>    /* clear_console(), putbytes() */
#include <malloc.h>     /*malloc() */
#include <physalloc.h>  /* test_physalloc() */
#include <scheduler.h>  /* scheduler_on_tick() */
#include <logger.h>		/* log_info() */
#include <task_manager.h>	/* task_manager_init() */
#include <keybd_driver.h>	/* readline() */
#include <lib_thread_management/sleep.h>		/* sleep_on_tick() */


volatile static int __kernel_all_done = 0;


/* Level of logging, set to 4 to turn logging off,
 * 1 to print logs for log priorities lo, med, hi
 * 2 to print logs for log priorities med, hi
 * 3 to print logs for log priorities hi
 *
 * defining the NDEBUG flag will also turn logging off
 */
int log_level = 1;

void tick(unsigned int numTicks) {
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

	clear_console();

	task_manager_init();
	//test_physalloc(); // TODO put in test suite


    //TODO: Run shell once exec and fork are working

	log("this is DEBUG");
	log_info("this is INFO");
	log_warn("this is WARN");

    while (!__kernel_all_done) {
		// Used for development to run a certain test straightaway
		//hard_code_test("exec_args_test");

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
