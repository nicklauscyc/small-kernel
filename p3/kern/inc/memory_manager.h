/** @brief Header file with VM API. */

#ifndef _MEMORY_MANAGER_H
#define _MEMORY_MANAGER_H

#include <elf_410.h> /* simple_elf_t */
#include <stdint.h> /* uint32_t */

int vm_init( void );
void *new_pd_from_elf( simple_elf_t *elf,
        uint32_t stack_lo, uint32_t stack_len );
void *new_pd_from_parent( void *parent_pd );
void vm_enable_task( void *ptd );
void enable_write_protection( void );
void disable_write_protection( void );
int vm_new_pages ( void *ptd, void *base, int len );

/* True if address if paged align, false otherwise */
#define PAGE_ALIGNED(address) ((((uint32_t) address) & (PAGE_SIZE - 1)) == 0)

#endif /* _MEMORY_MANAGER_H */
