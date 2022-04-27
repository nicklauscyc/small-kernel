/** @file atomic_utils.h
 *  @brief Atomic utilities. */

#ifndef ATOMIC_UTILS_H_
#define ATOMIC_UTILS_H_

#include <stdint.h> /* uin32_t */

/** @brief Compares and swaps value atomically.
 *
 *  @param at Location in which to compare and swap
 *  @param expect Value to expect at location
 *  @param new_val New value to set location to if it matches expectation
 *  @return Value at location. */
uint32_t compare_and_swap_atomic( uint32_t *at,
		uint32_t expect, uint32_t new_val );

/** @brief Adds one atomically to some value.
 *
 *  @param at Location to add one to
 *  @return Value at location before adding 1 */
uint32_t add_one_atomic( uint32_t *at );

#endif /* ATOMIC_UTILS_H_ */
