/** @file keybd_driver.h
 *  @brief Contains functions that can be called by interrupt handler assembly
 *         wrappers
 *
 *  @author Nicklaus Choo (nchoo)
 *  @bugs No known bugs.
 */

# ifndef _P1_KEYBD_DRIVER_H_
# define _P1_KEYBD_DRIVER_H_

#include <stdint.h>

void init_keybd(void);
void keybd_int_handler(void);
//int old_readline(char *buf, int len);

typedef int aug_char;
typedef uint8_t raw_byte;

/** @brief unbounded circular array heavily inspired by 15-122 unbounded array
 *
 *  first is the index of the earliest unread element in the buffer. Once
 *  all functions that will ever need to read a buffer element has read
 *  the element at index 'first', first++ modulo limit.
 *
 *  last is one index after the latest element added to the buffer. Whenever
 *  an element is added to the buffer, last++ modulo limit.
 */
typedef struct {
  uint32_t size; /* 0 <= size && size < limit */
  uint32_t limit; /* 0 < limit */
  uint32_t first; /* if first <= last, then size == last - first */
  uint32_t last; /* else last < first, then size == limit - first + last */
  raw_byte *data;
} uba;

int is_uba(uba *arr);
uba *uba_new(int limit);
uba *uba_resize(uba *arr);
void uba_add(uba *arr, uint8_t elem);
uint8_t uba_rem(uba *arr);
int uba_empty(uba *arr);

#endif
