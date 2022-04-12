/** @brief Header file with VM API. */

#ifndef _MEMORY_MANAGER_H
#define _MEMORY_MANAGER_H

#include <elf_410.h> /* simple_elf_t */
#include <stdint.h> /* uint32_t */

#define PAGE_DIRECTORY_INDEX 0xFFC00000
#define PAGE_TABLE_INDEX 0x003FF000

#define PAGE_DIRECTORY_SHIFT 22
#define PAGE_TABLE_SHIFT 12

/* Get page directory index from logical address */
#define PD_INDEX(addr) \
	((PAGE_DIRECTORY_INDEX & ((uint32_t)(addr))) >> PAGE_DIRECTORY_SHIFT)

/* Get page table index from logical address */
#define PT_INDEX(addr) \
	((PAGE_TABLE_INDEX & ((uint32_t)(addr))) >> PAGE_TABLE_SHIFT)

/* True if address if paged align, false otherwise */
#define PAGE_ALIGNED(address) ((((uint32_t) address) & (PAGE_SIZE - 1)) == 0)
#define USER_STR_LEN 256
#define NUM_USER_ARGS 16
#define TABLE_ADDRESS(PD_ENTRY) (((uint32_t) (PD_ENTRY)) & ~(PAGE_SIZE - 1))

#ifndef STACK_ALIGNED
#define STACK_ALIGNED(address) (((uint32_t) (address)) % 4 == 0)
#endif


void init_vm( void );
/** Whether page is read only or also writable. */
typedef enum write_mode write_mode_t;
enum write_mode { READ_ONLY, READ_WRITE };

void *new_pd_from_elf( simple_elf_t *elf,
        uint32_t stack_lo, uint32_t stack_len );
void *new_pd_from_parent( void *parent_pd );
void vm_enable_task( void *ptd );
void enable_write_protection( void );
void disable_write_protection( void );
int vm_new_pages ( void *ptd, void *base, int len );

int is_valid_pt( uint32_t *pt, int pd_index );
int is_valid_pd( void *pd );


int is_user_pointer_allocated( void *ptr );

int is_valid_user_pointer( void *ptr, write_mode_t write_mode );
int is_valid_user_string( char *s, int max_len );
int is_valid_null_terminated_user_string( char *s, int max_len );
int is_valid_user_argvec( char *execname,  char **argvec );
void free_pd_memory( void *pd );

int allocate_user_zero_frame( uint32_t **pd, uint32_t virtual_address,
							  uint32_t sys_prog_flag );
void unallocate_user_zero_frame( uint32_t **pd, uint32_t virtual_address);

void *get_pd( void );
int zero_page_pf_handler( uint32_t faulting_address );



#endif /* _MEMORY_MANAGER_H */
