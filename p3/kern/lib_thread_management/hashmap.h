/** @file hashmap.h
 *
 *  Definitions for hashmap.*/

#ifndef HASHMAP_H_
#define HASHMAP_H_

#include <stdint.h>         /* uint32_t */
#include <task_manager.h>   /* tcb_t */

/** @brief Struct containing information about a node inside the
 *         map's linked list.
 *
 *  @param val Thread status value of the node
 *  @param next Next node in the linked list */
typedef struct map_node {
    struct map_node *next;
    void *val;
    uint32_t key;
} map_node_t;

/* Number of buckets in hashmap. 1024 is large enough to
 * minimize collisions while avoiding being wasteful with
 * memory. */
#define NUM_BUCKETS 1024

/** @brief Struct containing the hashmaps buckets.
 *
 *  @param buckets Array with each linked list start
 *  @param num_buckets Length of buckets array */
typedef struct {
    map_node_t *buckets[NUM_BUCKETS];
} hashmap_t;

/* Hashmap (tid -> tcb) for use in thread library. */
hashmap_t map;

/* Hashmap functions */
void map_insert(uint32_t key, void *val);
void *map_get(uint32_t tid);
void *map_remove(uint32_t tid);
void map_init(void);

#endif /* HASHMAP_H_ */
