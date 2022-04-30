/** @file hashmap.c
 *  @brief A hashmap for use in thread bookkeeping
 *
 *  Hash function taken from https://github.com/skeeto/hash-prospector
 *
 *  @author Andre Nascimento (anascime) */

#include <stdint.h> /* uint32_t */
#include <stddef.h> /* NULL */
#include <stdlib.h> /* malloc, free */
#include <string.h> /* memset */
#include <assert.h> /* affirm */
#include <logger.h>
#include <task_manager_internal.h> /* Q MACRO for tcb */
#include <lib_thread_management/mutex.h>
#include <lib_thread_management/hashmap.h>

static mutex_t hashmux;

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
 *
 *  @param tcb Pointer to tcb to insert
 *  @return Void.
 *  */
void
map_insert( tcb_t *tcb )
{
	mutex_lock(&hashmux);
    uint32_t index = hash(tcb->tid) % NUM_BUCKETS;
    hashmap_queue_t *headp = &map.buckets[index];

	Q_INSERT_TAIL(headp, tcb, tid2tcb_queue);
	mutex_unlock(&hashmux);

}

/** @brief Get status in hashmap.
 *
 *  @param tid Id of thread status to look for
 *  @return Status, NULL if not found. */
tcb_t *
map_get(uint32_t tid)
{
	mutex_lock(&hashmux);

    uint32_t index = hash(tid) % NUM_BUCKETS;
    hashmap_queue_t *headp = &map.buckets[index];

	tcb_t *curr = Q_GET_FRONT(headp);
    while (curr && curr->tid != tid) {
        curr = Q_GET_NEXT(curr, tid2tcb_queue);
	}

	mutex_unlock(&hashmux);

    return curr;
}

/** @brief Remove status from hashmap.
 *
 *  @param tid Id of thread to remove
 *  @return Pointer to removed value on exit,
 *          NULL on failure.
 */
tcb_t *
map_remove(uint32_t tid)
{
	mutex_lock(&hashmux);

	log_info("map_remove(): remove tid:%d from tid2tcb hashmap", tid);
    uint32_t index = hash(tid) % NUM_BUCKETS;
    hashmap_queue_t *headp = &map.buckets[index];

	tcb_t *curr = Q_GET_FRONT(headp);

	while (curr) {
		if (curr->tid == tid) {
			Q_REMOVE(headp, curr, tid2tcb_queue);
			mutex_unlock(&hashmux);

			return curr;
		}
		curr = Q_GET_NEXT(curr, tid2tcb_queue);
	}
	mutex_unlock(&hashmux);

	return NULL;
}

/** @brief Initialize new hashmap
 *
 *  @return Void
 */
void
map_init( void )
{
	mutex_init(&hashmux);
    memset(map.buckets, 0, sizeof(hashmap_queue_t) * NUM_BUCKETS);
	for (int i=0; i < NUM_BUCKETS; ++i)
		Q_INIT_HEAD(&map.buckets[i]);
    return;
}

