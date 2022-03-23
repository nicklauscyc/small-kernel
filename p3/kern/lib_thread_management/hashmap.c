/** @file hashmap.c
 *  @brief A hashmap for use in thread bookkeeping
 *
 *  Hash function taken from https://github.com/skeeto/hash-prospector
 *
 *  @author Andre Nascimento (anascime) */

#include <lib_thread_management/hashmap.h>
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
static uint32_t
hash( uint32_t x )
{
    x ^= x >> 16;
    x *= 0x7feb352d;
    x ^= x >> 15;
    x *= 0x846ca68b;
    x ^= x >> 16;
    return x;
}

/** @brief Insert status into hashmap.
 *  @param map Hashmap in which to insert status
 *  @param tcb Pointer to tcb to insert
 *
 *  @return Void.
 *  */
void
map_insert( uint32_t key, void *val )
{
    uint32_t index = hash(key) % NUM_BUCKETS;
    map_node_t *curr = map.buckets[index];
    if (!curr) {
        curr = smalloc(sizeof(map_node_t));
        // FIXME: Cannot crash here, return -1 instead
        affirm(curr);
        curr->val = val;
        curr->key = key;
        curr->next = NULL;
        map.buckets[index] = curr;
        return;
    }

    while (curr->next != NULL)
        curr = curr->next;

    curr->next = smalloc(sizeof(map_node_t));
    affirm(curr->next);
    curr->next->val = val;
    curr->next->key = key;
    curr->next->next = NULL;
}

/** @brief Get status in hashmap.
 *  @param map Map from which to get status
 *  @param tid Id of thread status to look for
 *
 *  @return Status, NULL if not found. */
void *
map_get(uint32_t key)
{
    uint32_t index = hash(key) % NUM_BUCKETS;
    map_node_t *curr = map.buckets[index];

    while (curr && curr->key != key)
        curr = curr->next;

    if (curr)
        return curr->val;

    return NULL;
}

/** @brief Remove status from hashmap.
 *
 *  @param map Map from which to remove value of key
 *  @param key Key of value to remove
 *
 *  @return Pointer to removed value on exit,
 *          NULL on failure.
 * */
void *
map_remove(uint32_t key)
{
    uint32_t index = hash(key) % NUM_BUCKETS;
    map_node_t *curr = map.buckets[index];
    if (!curr)
        return NULL;

    if (curr->key == key) {
        map_node_t *to_remove = map.buckets[index];
        map.buckets[index] = to_remove->next;
        void *val = to_remove->val;
        sfree(to_remove, sizeof(map_node_t));
        return val;
    }

    while (1) {
        if (!curr->next)
            return NULL;
        if (curr->next->key == key) {
            map_node_t *to_remove = curr->next;
            curr->next = curr->next->next;
            void *val = to_remove->val;
            free(to_remove);
            return val;
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
void
map_init( void )
{
    memset(map.buckets, 0, sizeof(map_node_t *) * NUM_BUCKETS);
    return;
}

