/** @file scheduler.h
 *
 *  @brief Header file for scheduler module. Exports functions
 *  for others to interact with scheduling. */

#ifndef SCHEDULER_H_
#define SCHEDULER_H_

#include <task_manager.h>   /* status_t */
#include <stdint.h>         /* uint32_t */
#include <variable_queue.h> /* Q_NEW_HEAD() */

/** @brief A thread's status */
enum status { RUNNING, RUNNABLE, DESCHEDULED, BLOCKED, DEAD, UNINITIALIZED };
typedef enum status status_t;

/* Queue head definition */
Q_NEW_HEAD(queue_t, tcb);

int is_scheduler_init( void );
tcb_t *get_running_thread( void );
pcb_t *get_running_task( void );

int is_multi_threads( void );

void start_first_running_thread( void );

int get_running_tid( void );
void scheduler_on_tick( unsigned int num_ticks );
int make_thread_runnable( tcb_t *tcbp );
int switch_safe_make_thread_runnable( tcb_t *tcbp );
int yield_execution( status_t store_status, tcb_t *tcb,
		void (*callback)(tcb_t *, void *), void *data );

#endif /* SCHEDULER_H_ */
