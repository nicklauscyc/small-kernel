/** @brief Facilities for task creation and management.
 *  Also provides context switching functionallity. */

#ifndef _TASK_MANAGER_H
#define _TASK_MANAGER_H

#include <stdint.h> /* uint32_t */
#include <elf_410.h> /* simple_elf_t */
#include <task_manager_private.h> /* pcb_t tcb_t */

typedef struct pcb pcb_t;
typedef struct tcb tcb_t;

int find_tcb( int tid, tcb_t **tcb );

int create_new_task( int pid, int tid, simple_elf_t *elf );
int activate_task_memory( int pid );
void task_set_active( int tid, uint32_t esp, uint32_t entry_point );

// TODO temp for demo
int get_new_pcb( int pid, void *pd );
int get_new_tcb( int pid, int tid );

#endif /* _TASK_MANAGER_H */
