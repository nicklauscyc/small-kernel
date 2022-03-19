
#include <task_manager.h>

int get_running_tid( void );
int init_scheduler( tcb_t *tcb );
void add_tcb_to_run_queue(tcb_t *tcb);
void run_next_tcb( void );

