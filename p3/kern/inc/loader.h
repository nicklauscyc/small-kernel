/* The 15-410 kernel project
 *
 *     loader.h
 *
 * Structure definitions, #defines, and function prototypes
 * for the user process loader.
 */

#ifndef _LOADER_H
#define _LOADER_H


/* --- Prototypes --- */

int getbytes( const char *filename, int offset, int size, char *buf );

int execute_user_program( const char *fname, int argc, char **argv );

#endif /* _LOADER_H */
