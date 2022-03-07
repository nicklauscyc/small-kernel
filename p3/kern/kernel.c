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

#include <install_handler.h> /* handler_install() */
#include <common_kern.h>

/* libc includes. */
#include <stdio.h>
#include <simics.h>                 /* lprintf() */

/* multiboot header file */
#include <multiboot.h>              /* boot_info */

/* x86 specific includes */
#include <x86/asm.h>                /* enable_interrupts() */

#include <x86/cr.h> /* get_cr3() */

#include <console.h> /* clear_console(), putbytes() */
#include <keybd_driver.h> /* readline() */
#include <loader.h> /* execute_user_program() */

volatile static int __kernel_all_done = 0;

/* Think about where this declaration
 * should be... probably not here!
 */
void tick(unsigned int numTicks) {
  //lprintf("numTicks: %d\n", numTicks);
}

/** @brief Kernel entrypoint.
 *
 *  This is the entrypoint for the kernel.
 *
 * @return Does not return
 */
int kernel_main(mbinfo_t *mbinfo, int argc, char **argv, char **envp)
{
    // placate compiler
    (void)mbinfo;
    (void)argc;
    (void)argv;
    (void)envp;

	/* initialize device-driver library */
	int res = handler_install(tick);
	lprintf("res of handler_install: %d", res);



	clear_console();

    /*
     * When kernel_main() begins, interrupts are DISABLED.
     * You should delete this comment, and enable them --
     * when you are ready.
     */

    lprintf( "Hello from a brand new kernel!" );
	putbytes("executable user programs:\n", 26);
	putbytes("loader_test1\n", 13);
	putbytes("loader_test2\n", 13);
	putbytes("getpid_test1\n", 13);

	/* show cr3 */
	lprintf("cr1: 0x%p", (void *) get_cr3());
	lprintf("cr2: 0x%p", (void *) get_cr3());
	lprintf("cr3: 0x%p", (void *) get_cr3());
	lprintf("cr4: 0x%p", (void *) get_cr3());


    while (!__kernel_all_done) {
     	int n =  CONSOLE_HEIGHT * CONSOLE_WIDTH;
     	char s[n];

	 	/* Display prompt */
     	putbytes("pebbles>",8);
     	int res = readline(s, n);
	    lprintf("read %d bytes: \"%s\"", res, s);
		res = execute_user_program(s);
    }

    return 0;
}
