/** @file physalloc.c
 *  @brief Contains functions implementing interface functions for physalloc.h
 *
 *  A doubly linked list is used to store physical frames that have been
 *  used at least once but are currently free. This list is called reuse_list.
 *
 *  Whenever a new physical frame address is requested, first check the
 *  reuse_list for any free physical frame addresses. If there is one, return
 *  that free physical frame address. Else, return max_free_phys_address and
 *  update max_free_phys_address
 *
 *  @author Nicklaus Choo (nchoo)
 *  @bug No known bugs.
 */

#include <simics.h> /* MAGIC_BREAK */
#include <assert.h> /* affirm() */
#include <physalloc.h>
#include <variable_queue.h> /* Q_NEW_LINK() */
#include <malloc.h> /* malloc() */
#include <common_kern.h>/* USER_MEM_START */
#include <x86/page.h> /* PAGE_SIZE */

#define PHYS_FRAME_ADDRESS_ALIGNMENT(phys_address)\
	((phys_address & (PAGE_SIZE - 1)) == 0)

#define TOTAL_USER_FRAMES (machine_phys_frames() - (USER_MEM_START / PAGE_SIZE))

/* Boolean for whether initialization has taken place */
static int physalloc_init = 0;

/* List node to store a free physical frame address */
typedef struct free_frame_node {
	Q_NEW_LINK(free_frame_node) link;
	uint32_t phys_address;
} free_frame_node_t;

/* List head definition */
Q_NEW_HEAD(list_t, free_frame_node);

/* Address of the highest free physical frame currently in list */
static uint32_t max_free_address;

/* Number of allocated free physical frames >= USER_MEM_START */
/* # calls to physalloc() - # calls to physfree() */
static int num_alloc;

/* Linked list of free physical frames */
static list_t reuse_list;

/** @brief Returns number of free physical frames with physical address at
 *         least USER_MEM_START
 *
 *  @return Number of free physical frames
 */
uint32_t
num_free_phys_frames( void )
{
	return TOTAL_USER_FRAMES - num_alloc;
}

/** @brief Initializes physical allocator family of functions
 *
 *  Must be called once and only once
 *
 *  @param Void.
 */
void
init_physalloc( void )
{
	/* Initialize once and only once */
	affirm(!physalloc_init);

	/* Initialize queue head and add the first node into queue */
	Q_INIT_HEAD(&reuse_list);
	num_alloc = 0;

	/* Some essential checks that should never fail */
    assert(!Q_GET_TAIL(&reuse_list));
	assert(!Q_GET_FRONT(&reuse_list));

	max_free_address = USER_MEM_START;
	physalloc_init = 1;
}

/** @brief Allocates a physical frame by returning its address
 *
 *  If there is a reusable free frame, we use that frame first.
 *
 *  @return Next free physical frame address
 */
uint32_t
physalloc( void )
{
	/* First time running, initialize */
	if (!physalloc_init) {
		init_physalloc();
	}
	/* Check if there are still available physical frames >= USER_MEM_START */
    int remaining = TOTAL_USER_FRAMES - num_alloc;
	if (remaining <= 0) {

		/* Never have -ve remaining frames */
		assert(remaining == 0);

		/* Reuse list must be empty */
	    assert(!Q_GET_TAIL(&reuse_list));
		assert(!Q_GET_FRONT(&reuse_list));

		return 0;
	}
	/* Exist available physical frames, so check for reusable free frames */
	uint32_t next_free_address = 0;

	/* Reusable free frames are unallocated and thus not counted in num_alloc */
	if (Q_GET_FRONT(&reuse_list)) {
		free_frame_node_t *frontp = Q_GET_FRONT(&reuse_list);
		next_free_address = frontp->phys_address;
		assert(next_free_address <= max_free_address);

		/* Remove from list and free */
		Q_REMOVE(&reuse_list, frontp, link);
		free(frontp);

	/* Nothing to reuse, so return a new address */
	} else {
		next_free_address = max_free_address;
		max_free_address += PAGE_SIZE;
	}
	num_alloc++;

	/* Check alignment and return */
	assert(next_free_address);
	assert(PHYS_FRAME_ADDRESS_ALIGNMENT(next_free_address));
	return next_free_address;
}

/** @brief Frees a physical frame address
 *
 *  Does necessary but not sufficient checksto see if it is indeed a valid
 *  address before freeing (double freeing is not checked).
 *  Requires that this phys_address was returned from a call to
 *  physalloc(), else behavior is undefined. Free every allocated physical
 *  address exactly once.
 *
 *  @param phys_address Physical address to be freed.
 *  @return Void.
 */
void
physfree(uint32_t phys_address)
{
	affirm(PHYS_FRAME_ADDRESS_ALIGNMENT(phys_address));
	affirm(USER_MEM_START <= phys_address && phys_address <= max_free_address);

	/* Allocate and initialize new list node */
	free_frame_node_t *newp = malloc(sizeof(free_frame_node_t));
	Q_INIT_ELEM(newp, link);
	newp->phys_address = phys_address;

	/* Add to reuse list */
	Q_INSERT_FRONT(&reuse_list, newp, link);
	assert(Q_GET_FRONT(&reuse_list) == newp);
	num_alloc--;
}

/** @brief Tests physalloc and physfree
 *
 *  @return Void.
 */
void
test_physalloc( void )
{
	lprintf("Testing physalloc(), physfree()");
	uint32_t a, b, c;
	/* Quick test for alignment, we allocate in consecutive order */
	a = physalloc();
	assert(a == USER_MEM_START);
	b = physalloc();
	assert(b == a + PAGE_SIZE);

	/* Quick test for reusing free physical frames */
	physfree(a);
	c = physalloc(); /* c reuses a */
	assert(a == c);
	a = physalloc();
	assert(a == USER_MEM_START + 2 * PAGE_SIZE);

	/* Test reuse the latest freed phys frame */
	physfree(b);
	physfree(c);
	physfree(a);
	assert(physalloc() == a);
	assert(physalloc() == c);
	assert(physalloc() == b);
	physfree(USER_MEM_START + 2 * PAGE_SIZE);
	physfree(USER_MEM_START + 1 * PAGE_SIZE);
	physfree(USER_MEM_START);

	/* Use all phys frames */
	int total = TOTAL_USER_FRAMES;
	int i = 0;
	uint32_t all_phys[1024];
	lprintf("after all_phys");
	while (i < 1024) {
		all_phys[i] = physalloc();
		assert(all_phys[i]);
		i++;
		total--;
	}
	lprintf("total frames supported:%08x",
		    (unsigned int) TOTAL_USER_FRAMES);
	assert(total == TOTAL_USER_FRAMES - 1024);
	/* all phys frames, populate reuse list */
	assert(i == 1024);
	while (i > 0) {
		i--;
		physfree(all_phys[i]);
		total++;
	}
	assert(total == TOTAL_USER_FRAMES);
	assert(i == 0);

	/* exhaust reuse list, check implicit stack ordering of reuse list */
	while (i < 1024) {
		uint32_t addr = physalloc();
		assert(all_phys[i] == addr);
		assert(addr);
		if (i == 0) assert(all_phys[i] == USER_MEM_START);
		else assert(all_phys[i-1] + PAGE_SIZE == all_phys[i]);
		i++;
		total--;
	}
	/* free everything */
	while (i > 0) {
		i--;
		physfree(all_phys[i]);
		total++;
	}
	/*use ALL phys frames */
	assert(i == 0);
	assert(total == TOTAL_USER_FRAMES);
	uint32_t x = 0;
	while (total > 0) {
		total--;
		x = physalloc();
	}
	lprintf("last frame start address:%lx", x);
	assert(!physalloc());

	/* put them all back */
	lprintf("put all into linked list");
	while (total < TOTAL_USER_FRAMES) {
		total++;
		physfree(x);
		x -= PAGE_SIZE;
	}
	lprintf("Tests passed!");
}

