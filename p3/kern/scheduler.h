#ifndef SCHEDULER_H_
#define SCHEDULER_H_

#include <task_manager.h>   /* status_t */
#include <stdint.h>         /* uint32_t */

/* Queue head definition */
Q_NEW_HEAD(queue_t, tcb);

int is_scheduler_init( void );
int get_running_tid( void );
void scheduler_on_tick( unsigned int num_ticks );
int register_thread( uint32_t tid );
void add_to_runnable_queue( tcb_t *tcb );
int yield_execution( queue_t *store_at, status_t store_status, int tid );

#endif /* SCHEDULER_H_ */
