/** @file thr_internals.h
 *
 *  @brief This file may be used to define things
 *         internal to the thread library.
 */

#ifndef THR_INTERNALS_H
#define THR_INTERNALS_H

#include <stdint.h>

uint32_t add_one_atomic(uint32_t *at);

#endif /* THR_INTERNALS_H */
