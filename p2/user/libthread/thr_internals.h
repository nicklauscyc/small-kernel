/** @file thr_internals.h
 *
 *  @brief This file may be used to define things
 *         internal to the thread library.
 */

#ifndef THR_INTERNALS_H
#define THR_INTERNALS_H

#include <stdint.h>

/* Global variable set to 1 when thr_init() is called, 0 before */
extern int THR_INITIALIZED;

/** @brief Struct containing all necesary information about a thread.
 */
typedef struct {
	char *thr_stack_low;
	char *thr_stack_high;
	int tid;
	int runnable;
} thr_status_t;


uint32_t add_one_atomic(uint32_t *at);
int thread_fork(void *child_stack_start, void *(*func)(void *), void *arg);
void run_thread(void *rsp, void *(*func)(void *), void *arg);


/* Circular Unbounded Arrays */

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
  uint8_t *data;
} uba;

int is_uba(uba *arr);
uba *uba_new(int limit);
uba *uba_resize(uba *arr);
void uba_add(uba *arr, uint8_t elem);
uint8_t uba_rem(uba *arr);
int uba_empty(uba *arr);



#endif /* THR_INTERNALS_H */
