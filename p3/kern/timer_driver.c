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

#include <interrupt_defines.h> /* INT_CTL_PORT, INT_ACK_CURRENT */
#include <asm.h> /* outb() */
#include <assert.h> /* assert() */
#include <stddef.h> /* NULL */
#include <timer_defines.h> /* TIMER_SQUARE_WAVE */

/** @brief Get 8 least significant bits in x. */
#define LSB(x) ((long unsigned int)x & 0xFF)

/** @brief Get 8 most significant bits in x. */
#define MSB(x) (((long unsigned int)x >> 8) & 0xFF)

/* 1 khz. */
#define DESIRED_TIMER_RATE 1000

/* Initialize tick to NULL */
static void (*application_tickback) (unsigned int) = NULL;

/* Total ticks caught */
static unsigned int total_ticks = 0;

unsigned int
get_total_ticks( void )
{
	return total_ticks;
}

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
	uint32_t current_total_ticks = ++total_ticks;
	outb(INT_CTL_PORT, INT_ACK_CURRENT);
	application_tickback(current_total_ticks);
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
void init_timer(void (*volatile tickback)(unsigned int)) {

  /* Set application provided tickback function */
  assert(tickback != NULL);
  application_tickback = tickback;

  outb(TIMER_MODE_IO_PORT, TIMER_SQUARE_WAVE);
  uint16_t cycles_between_interrupts = (uint16_t)(TIMER_RATE / DESIRED_TIMER_RATE);

  outb(TIMER_PERIOD_IO_PORT, LSB(cycles_between_interrupts)); // Send lsb first
  outb(TIMER_PERIOD_IO_PORT, MSB(cycles_between_interrupts)); // then msb.

  return;
}




