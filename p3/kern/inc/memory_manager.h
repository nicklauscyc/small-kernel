/** @brief Header file with VM API. */

#ifndef _MEMORY_MANAGER_H
#define _MEMORY_MANAGER_H

#include <elf_410.h> /* simple_elf_t */
#include <stdint.h> /* uint32_t */

void init_vm( void );
void *new_pd_from_elf( simple_elf_t *elf,
        uint32_t stack_lo, uint32_t stack_len );
void *new_pd_from_parent( void *parent_pd );
void vm_enable_task( void *ptd );
void enable_write_protection( void );
void disable_write_protection( void );
int vm_new_pages ( void *ptd, void *base, int len );
int is_valid_pd( void *pd );
int is_user_pointer_valid( void *ptr );
int is_user_pointer_allocated( void *ptr );

int is_valid_user_string( char *s );
int is_valid_user_argvec( char *execname,  char **argvec );
void free_pd_memory( void *pd );

int allocate_user_zero_frame( uint32_t **pd, uint32_t virtual_address);
void unallocate_user_zero_frame( uint32_t **pd, uint32_t virtual_address);

void *get_pd( void );
int zero_page_pf_handler( uint32_t faulting_address );

/* True if address if paged align, false otherwise */
#define PAGE_ALIGNED(address) ((((uint32_t) address) & (PAGE_SIZE - 1)) == 0)
#define USER_STR_LEN 256
#define NUM_USER_ARGS 16
#define TABLE_ADDRESS(PD_ENTRY) (((uint32_t) (PD_ENTRY)) & ~(PAGE_SIZE - 1))

#endif /* _MEMORY_MANAGER_H */
