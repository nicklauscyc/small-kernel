/* The 15-410 kernel project
 *
 *     loader.h
 *
 * Structure definitions, #defines, and function prototypes
 * for the user process loader.
 */

#ifndef _LOADER_H
#define _LOADER_H

#include <elf_410.h>    /* simple_elf_t */

/* --- Prototypes --- */

int getbytes( const char *filename, int offset, int size, char *buf );

int execute_user_program( const char *fname, int argc, char **argv );

//TODO these should be reverted to static
int transplant_program_memory( simple_elf_t *se_hdr );
uint32_t * configure_stack( int argc, char **argv );

#endif /* _LOADER_H */
