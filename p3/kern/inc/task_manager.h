/** @brief Facilities for task creation and management.
 *  Also provides context switching functionallity. */

#ifndef TASK_MANAGER_H_
#define TASK_MANAGER_H_

#include <stdint.h>         /* uint32_t */
#include <elf_410.h>        /* simple_elf_t */
#include <variable_queue.h> /* Q_NEW_LINK */
//#include <lib_thread_management/mutex.h> /* mutex_t */

/* FIXME: Which of these do we really need? */
enum status { RUNNING, RUNNABLE, DESCHEDULED, BLOCKED, DEAD, UNINITIALIZED };
typedef enum status status_t;

typedef struct pcb pcb_t;
typedef struct tcb tcb_t;

Q_NEW_HEAD(thread_list_t, tcb);

/** @brief Task control block */
struct pcb {
    void *pd; // page directory
    uint32_t pid;
    int prepared; // Whether this task's VM has been initialized

	Q_NEW_LINK(pcb) task_link; // Embedded list of tasks

	//mutex_t	thread_list_mux;
	thread_list_t thread_list; // List of threads in this task
};

/** @brief Thread control block */
struct tcb {
	Q_NEW_LINK(tcb) thr_queue;
    Q_NEW_LINK(tcb) thread_link; // Embedded list of threads from same task
    status_t status;

    pcb_t *owning_task;

    uint32_t tid;

    /* Stack info. Needed for resuming execution.
     * General purpose registers, program counter
     * are stored on stack pointed to by esp. */
    uint32_t *kernel_stack_hi; /* Highest _writable_ address in kernel stack */
	                           /* that is stack aligned */
    uint32_t *kernel_esp; // needed for context switch
	uint32_t *kernel_stack_lo;
};


tcb_t *find_tcb( uint32_t tid );
pcb_t *find_pcb( uint32_t pid );
int create_pcb( uint32_t *pid, void *pd );
int create_tcb( uint32_t pid , uint32_t *tid );

void task_manager_init( void );
int create_task( uint32_t *pid, uint32_t *tid, simple_elf_t *elf );
int activate_task_memory( uint32_t pid );
void task_set_active( uint32_t tid, uint32_t esp, uint32_t entry_point );

#endif /* TASK_MANAGER_H_ */
