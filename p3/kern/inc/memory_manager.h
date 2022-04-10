/** @brief Header file with VM API. */

#ifndef _MEMORY_MANAGER_H
#define _MEMORY_MANAGER_H

#include <elf_410.h> /* simple_elf_t */
#include <stdint.h> /* uint32_t */

/** Whether page is read only or also writable. */
typedef enum write_mode write_mode_t;
enum write_mode { READ_ONLY, READ_WRITE };

int vm_init( void );
void *new_pd_from_elf( simple_elf_t *elf,
        uint32_t stack_lo, uint32_t stack_len );
void *new_pd_from_parent( void *parent_pd );
void vm_enable_task( void *ptd );
void enable_write_protection( void );
void disable_write_protection( void );
int vm_new_pages ( void *ptd, void *base, int len );
int is_valid_pd( void *pd );
int is_valid_user_pointer( void *ptr, write_mode_t write_mode );
int is_valid_user_string( char *s, int max_len );
int is_valid_null_terminated_user_string( char *s, int max_len );
int is_valid_user_argvec( char *execname,  char **argvec );
void free_pd_memory( void *pd );

/* True if address if paged align, false otherwise */
#define PAGE_ALIGNED(address) ((((uint32_t) address) & (PAGE_SIZE - 1)) == 0)
#define USER_STR_LEN 256
#define NUM_USER_ARGS 16

#endif /* _MEMORY_MANAGER_H */
