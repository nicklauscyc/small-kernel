/** @file task_manager.h
 *  @brief Facilities for task creation and management.
 *
 *  @author Andre Nascimento (anascime)
 *  @author Nicklaus Choo (nchoo)
 */

#ifndef TASK_MANAGER_H_
#define TASK_MANAGER_H_

#include <stdint.h>    /* uint32_t */
#include <elf_410.h>   /* simple_elf_t */

#define KERNEL_THREAD_STACK_SIZE (PAGE_SIZE)
#define USER_THREAD_STACK_SIZE (2 * PAGE_SIZE)

typedef enum status status_t;

/* PCB and TCB data structures */
typedef struct pcb pcb_t;
typedef struct tcb tcb_t;



/* Functions for task and thread creation */
void task_manager_init( void );
int create_pcb( uint32_t *pid, void *pd, pcb_t *parent_pcb );
int create_tcb( uint32_t pid , uint32_t *tid );
int create_task( uint32_t *pid, uint32_t *tid, simple_elf_t *elf );
int activate_task_memory( uint32_t pid );
void task_set_active( uint32_t tid );
void task_start( uint32_t tid, uint32_t esp, uint32_t entry_point );

/* Utility functions for getting and setting task and thread information */
tcb_t *find_tcb( uint32_t tid );
pcb_t *find_pcb( uint32_t pid );
uint32_t get_pid( void );
status_t get_tcb_status( tcb_t *tcb );
uint32_t get_tcb_tid(tcb_t *tcb);
void set_task_exit_status( int status );

int get_num_active_threads_in_owning_task( tcb_t *tcbp );
void *get_kern_stack_lo( tcb_t *tcbp );
void *get_kern_stack_hi( tcb_t *tcbp );
void set_kern_esp( tcb_t *tcbp, uint32_t *kernel_esp );
void *swap_task_pd( void *new_pd );
void *get_tcb_pd(tcb_t *tcb);
void free_tcb(tcb_t *tcb);
void free_pcb_but_not_pd(pcb_t *pcb);

#endif /* TASK_MANAGER_H_ */
