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

int execute_user_program( const char *fname, int argc, char **argv,
                          int replace_current_task );

#endif /* _LOADER_H */
