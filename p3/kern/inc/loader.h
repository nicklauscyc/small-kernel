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

int execute_user_program( char *fname, int argc, char **argv);

int load_initial_user_program( char *fname, int argc, char **argv );

#endif /* _LOADER_H */
