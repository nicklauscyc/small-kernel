/** @file hashmap.h
 *
 *  Definitions for thread hashmap.*/

#ifndef HASHMAP_H_
#define HASHMAP_H_

#include <stdint.h>         /* uint32_t */
#include <task_manager.h>   /* tcb_t */

/* Number of buckets in hashmap. 1024 is large enough to
 * minimize collisions while avoiding being wasteful with
 * memory. */
#define NUM_BUCKETS 1024

Q_NEW_HEAD(hashmap_queue_t, tcb);

/** @brief Struct containing the hashmaps buckets.
 *
 *  @param buckets Array with each linked list start
 *  @param num_buckets Length of buckets array */
typedef struct {
    hashmap_queue_t buckets[NUM_BUCKETS];
} hashmap_t;

/* Hashmap (tid -> tcb) for use in thread library. */
hashmap_t map;

/* Hashmap functions */
void map_insert(tcb_t *tcb);
tcb_t *map_get(uint32_t tid);
tcb_t *map_remove(uint32_t tid);
void map_init(void);

#endif /* HASHMAP_H_ */
