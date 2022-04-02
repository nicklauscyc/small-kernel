/** @file task_manager.h
 *  @brief Facilities for task creation and management.
 *
 *  @author Andre Nascimento (anascime)
 *  @author Nicklaus Choo (nchoo)
 */

#ifndef TASK_MANAGER_H_
#define TASK_MANAGER_H_

#include <stdint.h>                /* uint32_t */
#include <elf_410.h>               /* simple_elf_t */
#include <task_manager_internal.h> /* pcb_t tcb_t */

/* PCB and TCB data structures */
typedef struct pcb pcb_t;
typedef struct tcb tcb_t;

/* Functions for task and thread creation */
int create_pcb( uint32_t *pid, void *pd );
int create_tcb( uint32_t pid , uint32_t *tid );
int create_task( uint32_t *pid, uint32_t *tid, simple_elf_t *elf );
int activate_task_memory( uint32_t pid );
void task_set_active( uint32_t tid, uint32_t esp, uint32_t entry_point );

/* Utility functions for getting and setting task and thread information */
tcb_t *find_tcb( uint32_t tid );
pcb_t *find_pcb( uint32_t pid );
int get_num_threads_in_owning_task( tcb_t *tcbp );
void *get_kern_stack_hi( tcb_t *tcbp );
void set_kern_esp( tcb_t *tcbp, uint32_t *kernel_esp );

#endif /* TASK_MANAGER_H_ */
