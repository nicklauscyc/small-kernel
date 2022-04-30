/** @file timer_driver.h
 *  @brief Contains internal functions that implement the timer driver.
 *
 *  This header is to be included by install_handler.c
 *
 *  @author Nicklaus Choo (nchoo)
 *  @bug No known bugs.
 */

#ifndef P1_TIMER_DRIVER_H_
#define P1_TIMER_DRIVER_H_

void init_timer( void (*tickback)(unsigned int) );
unsigned int get_total_ticks( void );

#endif /* P1_TIMER_DRIVER_H_ */



