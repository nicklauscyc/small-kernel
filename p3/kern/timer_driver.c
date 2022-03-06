/** @file timer_driver.c
 *  @brief Contains functions that implement the timer driver
 *
 *  The PC timer rate is 1193182 Hz. The timer is configured to generate
 *  interrupts every 10 ms. and so we round off to an interrupt every
 *  11932 clock cycles. (which is more accurate then rounding down to
 *  11931 clock cycles.
 *
 *  @author Nicklaus Choo (nchoo)
 */

#include <x86/interrupt_defines.h> /* INT_CTL_PORT, INT_ACK_CURRENT */
#include <x86/asm.h> /* outb() */
#include <assert.h> /* assert() */
#include "stddef.h" /* NULL */
#include <x86/timer_defines.h> /* TIMER_SQUARE_WAVE */

#define INTERRUPT 100
#define SHORT_LSB_MASK 0x00FF
#define SHORT_MSB_MASK 0xFF00

/* Initialize tick to NULL */
static void (*application_tickback) (unsigned int) = NULL;

/* Total ticks caught */
static unsigned int total_ticks = 0;

/*********************************************************************/
/*                                                                   */
/* Internal helper functions                                         */
/*                                                                   */
/*********************************************************************/

/** @brief Update total number of timer interrupts received and call the
 *         application provided timer callback function.
 *
 *  The interface for the application provided timer callback as written in the
 *  handout says that 'Timer callbacks should run "quickly"', and so
 *  timer_int_handler() waits for the callback application_tickback() to
 *  return quickly before sending an ACK to the relevant I/O port and returning.
 *
 *  @return Void.
 */
void timer_int_handler(void) {

  /* Update total ticks */
  total_ticks += 1;

  /* Pass total ticks to application callback which should run quickly */
  application_tickback(total_ticks);

  /* Acknowledge interrupt and return */
  outb(INT_CTL_PORT, INT_ACK_CURRENT);
  return;
}

/** @brief Initializes the timer driver
 *
 *  TIMER_RATE is the clock cycles per second. To generate interrupts once
 *  every 10 ms, we generate 1 interrupt every 10/1000 s which is 1/100 s.
 *  Therefore the number of timer cycles between interrupts is:
 *
 *  TIMER_RATE cycles     1 s
 *  ----------------- x ------ = TIMER_RATE / 100 cycles
 *         s             100
 *
 *  @param tickback Application provided function for callbacks triggered by
 *         timer interrupts.
 *  @return Void.
 */
void init_timer(void (*tickback)(unsigned int)) {

  assert(tickback != NULL);

  outb(TIMER_MODE_IO_PORT, TIMER_SQUARE_WAVE);
  short cycles_between_interrupts = (short)(TIMER_RATE / INTERRUPT);

  /* Round off */
  if ((TIMER_RATE % INTERRUPT) > (INTERRUPT / 2)) {
    cycles_between_interrupts += 1;

    assert(((cycles_between_interrupts - 1) * INTERRUPT) +
        (TIMER_RATE % INTERRUPT) == TIMER_RATE);
  } else {
    assert((cycles_between_interrupts * INTERRUPT) +
        (TIMER_RATE % INTERRUPT) == TIMER_RATE);
  }
  /* Send the least significant byte */
  short lsb =  cycles_between_interrupts & SHORT_LSB_MASK;
  outb(TIMER_PERIOD_IO_PORT, lsb);

  /* Send the most significant byte */
  short msb = (cycles_between_interrupts & SHORT_MSB_MASK) >> 8;
  outb(TIMER_PERIOD_IO_PORT, msb);

  /* Set application provided tickback function */
  application_tickback = tickback;

  return;
}




