/** @brief Module for scheduling of threads. */

#ifndef SCHEDULER_H_
#define SCHEDULER_H_

#include <task_manager.h>
#include <stdint.h>

void
register_new_thread ( uint32_t );

uint32_t
get_active_thread_tid ( void );

void
scheduler_on_timer_interrupt( int total_ticks );

#endif /* SCHEDULER_H_ */
