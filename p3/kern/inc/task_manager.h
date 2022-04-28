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

#define KERNEL_THREAD_STACK_SIZE (2 * PAGE_SIZE)
#define USER_THREAD_STACK_SIZE (2 * PAGE_SIZE)

typedef enum status status_t;

/* PCB and TCB data structures */
typedef struct pcb pcb_t;
typedef struct tcb tcb_t;

/* Functions for task and thread creation */
void task_manager_init( void );
pcb_t *create_pcb( uint32_t *pid, void *pd, pcb_t *parent_pcb );
tcb_t *create_tcb( pcb_t *owning_task, uint32_t *tid );
int create_task( uint32_t *pid, uint32_t *tid, simple_elf_t *elf );
void activate_task_memory( pcb_t *pcb );
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
void free_pcb_but_not_pd_no_last_thread( pcb_t *pcb );

void remove_pcb( pcb_t *pcbp );
pcb_t *get_init_pcbp( void );
void register_if_init_task( char *execname, uint32_t pid );

void set_task_name( pcb_t *pcbp, char *execname );

uint32_t get_user_eflags( void );

void set_running_thread( tcb_t *tcbp );

#endif /* TASK_MANAGER_H_ */
