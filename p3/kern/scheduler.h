#ifndef SCHEDULER_H_
#define SCHEDULER_H_

#include <task_manager.h>
#include <stdint.h> /* uint32_t */

int get_running_tid( void );
int init_scheduler( uint32_t tid );
void scheduler_on_tick( unsigned int num_ticks );
int register_thread( uint32_t tid );
void add_to_runnable_queue( tcb_t *tcb );

#endif /* SCHEDULER_H_ */
