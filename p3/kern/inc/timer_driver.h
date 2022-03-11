/** @file timer_driver.h
 *  @brief Contains internal functions that implement the timer driver.
 *
 *  This header is to be included by install_handler.c
 *
 *  @author Nicklaus Choo (nchoo)
 *  @bug No known bugs.
 */

#ifndef _P1_TIMER_DRIVER_H_
#define _P1_TIMER_DRIVER_H_

void init_timer(void (*tickback)(unsigned int));

#endif



