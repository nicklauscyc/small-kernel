/** @brief Facilities for task creation and management.
 *  Also provides context switching functionallity. */

#ifndef _TASK_MANAGER_H
#define _TASK_MANAGER_H

#include <stdint.h> /* uint32_t */
#include <elf_410.h> /* simple_elf_t */
#include <task_manager_private.h> /* pcb_t tcb_t */

typedef struct pcb pcb_t;
typedef struct tcb tcb_t;


int task_new( int pid, int tid, simple_elf_t *elf );
int task_prepare( int pid );
void task_set( int tid, uint32_t esp, uint32_t entry_point );
void task_switch( int pid );

#endif /* _TASK_MANAGER_H */
