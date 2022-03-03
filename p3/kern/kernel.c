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

#include <common_kern.h>

/* libc includes. */
#include <stdio.h>
#include <simics.h>                 /* lprintf() */

/* multiboot header file */
#include <multiboot.h>              /* boot_info */

/* x86 specific includes */
#include <x86/asm.h>                /* enable_interrupts() */


#include <console.h> /* clear_console() */

volatile static int __kernel_all_done = 0;

//int readline(char *buf, int len);

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

	clear_console();

    /*
     * When kernel_main() begins, interrupts are DISABLED.
     * You should delete this comment, and enable them --
     * when you are ready.
     */

    lprintf( "Hello from a brand new kernel!" );

    while (!__kernel_all_done) {
     //	int n =  CONSOLE_HEIGHT * CONSOLE_WIDTH;
     //	char s[n];

	 //	/* Display prompt */
     //	putbytes("pebbles>",8);
     //	//int res = readline(s, n);
	 //   //lprintf("read %d bytes: \"%s\"", res, s);
     //	continue;
    }

    return 0;
}
