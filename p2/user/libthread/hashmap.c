/** @file hashmap.c
 *  @brief A hashmap for use in thread bookkeeping
 *
 *  Hash function taken from https://github.com/skeeto/hash-prospector
 *
 *  @author Andre Nascimento (anascime) */

#include <thr_internals.h>
#include <stdint.h> /* uint32_t */
#include <stddef.h> /* NULL */
#include <stdlib.h> /* malloc, free */
#include <string.h> /* memset */
#include <assert.h> /* affirm */

/** @brief Hash for placement into map
 *
 *  @param x Key to be hashed
 *  @return Hash.
 *  */
static uint32_t hash(uint32_t x) {
    x ^= x >> 16;
    x *= 0x7feb352d;
    x ^= x >> 15;
    x *= 0x846ca68b;
    x ^= x >> 16;
    return x;
}

/** @brief Insert status into hashmap.
 *  @param map Hashmap in which to insert status
 *  @param tstatusp Pointer to status to insert
 *
 *  @return Void.
 *  */
void insert(hashmap_t *map, thr_status_t *tstatusp) {
    uint32_t index = hash(tstatusp->tid) % map->num_buckets;
    map_node_t *curr = map->buckets[index];
    if (!curr) {
        curr = malloc(sizeof(map_node_t));
        affirm(curr);
        curr->val = tstatusp;
        curr->next = NULL;
        map->buckets[index] = curr;
        return;
    }

    while (curr->next != NULL)
        curr = curr->next;

    curr->next = malloc(sizeof(map_node_t));
    affirm(curr->next);
    curr->next->val = tstatusp;
    curr->next->next = NULL;
}

/** @brief Get status in hashmap.
 *  @param map Map from which to get status
 *  @param tid Id of thread status to look for
 *
 *  @return Status, NULL if not found. */
thr_status_t *get(hashmap_t *map, int tid) {
    uint32_t index = hash(tid) % map->num_buckets;
    map_node_t *curr = map->buckets[index];

    while (curr && curr->val->tid != tid)
        curr = curr->next;

    if (curr)
        return curr->val;

    return NULL;
}

/** @brief Remove status from hashmap.
 *
 *  @param map Map from which to remove status
 *  @param tid Id of thread whose status we wish to remove
 *
 *  @return Pointer to removed status on exit,
 *          negative number on failure.
 * */
thr_status_t *remove(hashmap_t *map, int tid) {
    uint32_t index = hash(tid) % map->num_buckets;
    map_node_t *curr = map->buckets[index];
    if (!curr)
        return NULL;

    if (curr->val->tid == tid) {
        map_node_t *to_remove = map->buckets[index];
        map->buckets[index] = to_remove->next;
        thr_status_t *statusp = to_remove->val;
        free(to_remove);
        return statusp;
    }

    while (1) {
        if (!curr->next)
            return NULL;
        if (curr->next->val->tid == tid) {
            map_node_t *to_remove = curr->next;
            curr->next = curr->next->next;
            thr_status_t *statusp = to_remove->val;
            free(to_remove);
            return statusp;
        }
        curr = curr->next;
    }
    return NULL;
}

/** @brief Initialize new hashmap
 *  @param map Memory location in which to initialize
 *  @param num_buckets Number of buckets for this map
 *
 *  @return 0 on success, -1 on failure
 *  */
int new_map(hashmap_t *map, unsigned int num_buckets) {
    if (!map || num_buckets == 0)
        return -1;

    map->buckets = malloc(sizeof(map_node_t) * num_buckets);
    if (!map->buckets)
        return -1;

    memset(map->buckets, 0, sizeof(map_node_t) * num_buckets);
    map->num_buckets = num_buckets;
    return 0;
}

/** @brief Destroy hashmap
 *  @param map Map to destroy
 *
 *  @return Void.
 *  */
void destroy_map(hashmap_t *map) {
    if (!map || !map->buckets)
        return;
    free(map->buckets);
}
