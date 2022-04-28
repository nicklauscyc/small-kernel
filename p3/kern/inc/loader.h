/** @file loader.h
 *  @brief Definitions for the process loader
 */

#ifndef LOADER_H_
#define LOADER_H_

#include <elf_410.h>    /* simple_elf_t */

int getbytes( const char *filename, int offset, int size, char *buf );

int execute_user_program( char *fname, char **argv);

int load_initial_user_program( char *fname, int argc, char **argv );

#endif /* LOADER_H_ */
