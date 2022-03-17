/** @file physalloc.h
 *  @brief Contains the interface for allocating and freeing physical frames.
 *
 *  Note that physical frame addresses are always represented as uint32_t so
 *  we never have to worry about accidentally dereferencing them when paging
 *  is turned on.
 *
 *  @author Nicklaus Choo (nchoo)
 *  @bug No known bugs.
 */

#ifndef _PHYSALLOC_H_
#define _PHYSALLOC_H_

#include <stdint.h> /* uint32_t */

/* Function prototypes */
uint32_t physalloc( void );
void physfree( uint32_t phys_address );

/* Test functions */
void test_physalloc( void );

#endif /* _PHYSALLOC_H_ */
