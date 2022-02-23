/** @file thr_internals.h
 *
 *  @brief This file may be used to define things
 *         internal to the thread library.
 */

#ifndef THR_INTERNALS_H
#define THR_INTERNALS_H

#include <stdint.h> /* uint32_t */
#include <cond_type.h> /* cond_t */
#include <syscall.h>
#include <stdarg.h> /* va_list */

void tprintf( const char *format, ... );
void vtprintf( const char *format, va_list args );

/* Global variable set to 1 when thr_init() is called, 0 before */
extern volatile int THR_INITIALIZED;

extern void *global_stack_low;

/** @brief Struct containing all necesary information about a thread.
 *
 *  @param thr_stack_low Lowest valid memory address in thread stack
 *  @param thr_stack_high Highest valid memory address in thread stack
 *  @param tid Thread ID (matching that of gettid() syscall)
 *  @param exit_cvar A cvar which will receive a broadcast on thr_exit
 *  @param exited Whether the thread has exited
 *  @param statusp Status which the thread exited with. Valid iff exited == 1
 */
typedef struct {
	char *thr_stack_low;
	char *thr_stack_high;
	int tid;
    int exited;
    void *status;
    cond_t *exit_cvar;
	cond_t _exit_cvar;
} thr_status_t;

extern thr_status_t root_tstatus;
extern thr_status_t *root_tstatusp;

uint32_t add_one_atomic(uint32_t *at);
int thread_fork(void *child_stack_start, void *(*func)(void *), void *arg);
void run_thread(void *rsp, void *(*func)(void *), void *arg);

/* Hashmap */
#define NUM_BUCKETS 1024

/** @brief Struct containing information about a node inside the
 *         map's linked list.
 *
 *  @param val Thread status value of the node
 *  @param next Next node in the linked list */
typedef struct map_node {
    thr_status_t *val;
    struct map_node *next;
} map_node_t;

/** @brief Struct containing the hashmaps buckets.
 *
 *  @param buckets Array with each linked list start
 */
typedef struct {
    map_node_t *buckets[NUM_BUCKETS];
} hashmap_t;

void init_map( void );
void insert( thr_status_t *tstatusp );
thr_status_t *get( int tid );
thr_status_t *remove( int tid );

#endif /* THR_INTERNALS_H */
