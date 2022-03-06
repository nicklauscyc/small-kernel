/** @file game.c
 *  @brief A kernel with timer, keyboard, console support which serves mainly
 *         as a big file of test functions.
 *
 *  This file contains the kernel's main() function.
 *
 *  It sets up the drivers and starts the game. It contains test code to
 *  exercise the kernel
 *
 *  @author Nicklaus Choo (nchoo)
 *  @bug No known bugs.
 */
#include <assert.h>
#include "../spec/p1kern.h"

/* libc includes. */
#include <stdio.h>
#include <simics.h>                 /* lprintf() */
#include <malloc.h>

/* multiboot header file */
#include <multiboot.h>              /* boot_info */

/* memory includes. */
#include <lmm.h>                    /* lmm_remove_free() */

/* x86 specific includes */
#include <x86/seg.h>                /* install_user_segs() */
#include <x86/interrupt_defines.h>  /* interrupt_setup() */
#include <x86/asm.h>                /* enable_interrupts() */

#include <string.h>

volatile static int __kernel_all_done = 0;

/* Think about where this declaration
 * should be... probably not here!
 */
void tick(unsigned int numTicks) {
  //lprintf("numTicks: %d\n", numTicks);
}

void test_scroll(void) {
  for (size_t i = 0; i < CONSOLE_HEIGHT + 1; i++) {
    printf("%d\n", i);
  }
}
void test_putbyte(void) {
  for (size_t i = 0; i < CONSOLE_HEIGHT; i++) {
    draw_char(i, 1, (i % 10) + '0', FGND_RED);
  }
  printf("a");
}

/** @brief Runs a few assert statements to test draw_char() and
 *         get_char() functions
 *
 *
 */
void test_draw_char_get_char(void) {
  lprintf("Testing draw_char() and get_char()");
  draw_char(0,0, 'A', FGND_RED | BGND_BLACK);
  assert(get_char(0,0) == 'A');

  draw_char(0, CONSOLE_WIDTH - 1, 'B', FGND_WHITE | BGND_BLUE);
  assert(get_char(0, CONSOLE_WIDTH - 1) == 'B');

  draw_char(CONSOLE_HEIGHT - 1, CONSOLE_WIDTH - 1, 'C', FGND_RED | BGND_GREEN);
  assert(get_char(CONSOLE_HEIGHT - 1, CONSOLE_WIDTH - 1) == 'C');

  draw_char(CONSOLE_HEIGHT - 1, 0, 'D', FGND_YLLW | BGND_CYAN);
  assert(get_char(CONSOLE_HEIGHT - 1, 0) == 'D');

  /* Offscreen row drawing has no effect */
  draw_char(CONSOLE_HEIGHT, 0, 'E', FGND_YLLW | BGND_CYAN);
  assert(get_char(CONSOLE_HEIGHT, 0) == '\0');

  /* Offscreen col drawing has no effect */
  draw_char(CONSOLE_HEIGHT - 1, CONSOLE_WIDTH, 'E', FGND_YLLW | BGND_CYAN);
  assert(get_char(CONSOLE_HEIGHT - 1, CONSOLE_WIDTH) == '\0');

  /* Invalid background color drawing has no effect */
  draw_char(CONSOLE_HEIGHT - 1, 0, 'F', FGND_YLLW | 0x190);
  assert(get_char(CONSOLE_HEIGHT - 1, 0) == 'D');

  /* Invalid unprintable character drawing has becomes ? */
  draw_char(CONSOLE_HEIGHT - 1, 0, '\6', FGND_YLLW | BGND_RED);
  assert(get_char(CONSOLE_HEIGHT - 1, 0) == 'D');

  lprintf("Passed draw_char() and get_char()");
  return;
}

void test_cursor(void) {

  lprintf("Testing: set_cursor()");
  /* Out of bounds checks */
  assert(set_cursor(CONSOLE_HEIGHT, 0) == -1);
  assert(set_cursor(-1, 0) == -1);
  assert(set_cursor(0, CONSOLE_WIDTH) == -1);
  assert(set_cursor(0, -1) == -1);

  /* In bounds checks */
  assert(set_cursor(0, 0) == 0);
  assert(set_cursor(0, CONSOLE_WIDTH -1) == 0);
  assert(set_cursor(CONSOLE_HEIGHT - 1, CONSOLE_WIDTH - 1)
         == 0);
  assert(set_cursor(CONSOLE_HEIGHT - 1, 0)
         == 0);
  assert(set_cursor(0, 0) == 0);
  lprintf("Passed: set_cursor()");
}


/** @brief Kernel entrypoint.
 *
 *  This is the entrypoint for the kernel.  It simply sets up the
 *  drivers and passes control off to game_run().
 *
 * @return Does not return
 */
int kernel_main(mbinfo_t *mbinfo, int argc, char **argv, char **envp)
{
    /*
     * Initialize device-driver library.
     */
    int res = handler_install(tick);
    lprintf("res of handler_install: %d", res);

    /*
     * When kernel_main() begins, interrupts are DISABLED.
     * You should delete this comment, and enable them --
     * when you are ready.
     *
     */
    printf("h");

    lprintf( "Hello from a brand new kernel!" );


    char * badguy = (char *) 0xdeadd00d;
    char * s = "hello";
    lprintf("badguy: 0x%08lx, s: 0x%08lx", (long) badguy, (long) s);
    //putbytes((char *), 3);


    test_draw_char_get_char();
    //test_putbyte();

    test_cursor();

    //clear_console();

    //printf("Hello Mom!");
    //
    test_scroll();

    clear_console();

    putbytes("carriage return should bring the cursor to the front of this line.\r\n",67);

    int tohide = 0;
    while (!__kernel_all_done) {
      if (tohide) hide_cursor();
      else show_cursor();
       int n =  CONSOLE_HEIGHT * CONSOLE_WIDTH;
       char s[n];
       int res = readline(s, n);
       lprintf("characters read: %d, '%s'",res, s);
       putbytes("how many characters to read next",33);
       res = readline(s, n);
       lprintf("characters read: %d, '%s'",res, s);


       tohide = !tohide;

      continue;
    }

    return 0;
}

