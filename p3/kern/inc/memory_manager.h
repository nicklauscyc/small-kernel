/** @brief Header file with VM API. */

#ifndef _MEMORY_MANAGER_H
#define _MEMORY_MANAGER_H

#include <elf_410.h> /* simple_elf_t */
#include <stdint.h> /* uint32_t */

int vm_init( void );
void *get_new_pd(simple_elf_t *elf, uint32_t stack_lo,
                         uint32_t stack_len );
void vm_enable_task( void *ptd );
void enable_write_protection( void );
void disable_write_protection( void );
int vm_new_pages ( void *ptd, void *base, int len );

#endif /* _MEMORY_MANAGER_H */
