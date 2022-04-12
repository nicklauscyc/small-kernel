/** @file atomic_utils.h
 *  @brief Atomic utilities. */

#ifndef ATOMIC_UTILS_H_
#define ATOMIC_UTILS_H_

#include <stdint.h> /* uin32_t */

uint32_t compare_and_swap_atomic( uint32_t *at,
		uint32_t expect, uint32_t new_val );
uint32_t add_one_atomic( uint32_t *at );

#endif /* ATOMIC_UTILS_H_ */
