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

#include <physalloc.h>

#include <string.h>         /* memcpy() */
#include <assert.h>         /* affirm() */
#include <variable_queue.h> /* Q_NEW_LINK() */
#include <malloc.h>         /* malloc() */
#include <common_kern.h>    /* USER_MEM_START */
#include <page.h>           /* PAGE_SIZE */
#include <logger.h>         /* log */
#include <lib_thread_management/mutex.h> /* mutex_t */

#define PHYS_FRAME_ADDRESS_ALIGNMENT(phys_address)\
	((phys_address & (PAGE_SIZE - 1)) == 0)

/** Number of pages as of yet unclaimed from system.
 *  Equals to machine_phys_frames() - (max_free_address / PAGE_SIZE) */
#define TOTAL_USER_FRAMES (machine_phys_frames() - (USER_MEM_START / PAGE_SIZE))

/** Number of pages as of yet unclaimed from system.
 *  Equals to machine_phys_frames() - (max_free_address / PAGE_SIZE) */
#define UNCLAIMED_PAGES (machine_phys_frames() - (max_free_address / PAGE_SIZE))

/* Whether physical frame allocater is initialized */
static int physalloc_init = 0;

/* Address of the highest free physical frame currently in list */
static uint32_t max_free_address;

typedef struct {
    uint32_t top; /* Index of next empty address in array */
    uint32_t len;
    uint32_t *data;
} stack_t;

static stack_t reuse_stack;

static mutex_t mux;

/** @brief Checks if a physical address is page aligned and could have
 *         been given out by physalloc
 *
 *  @param physaddress Physical address
 *  @return 1 if valid, 0 otherwise
 */
int
is_physframe( uint32_t phys_address )
{
	if (!PHYS_FRAME_ADDRESS_ALIGNMENT(phys_address)) {
		log_warn("0x%08lx is not page aligned!", phys_address);
		return 0;
	}
	if (!(USER_MEM_START <= phys_address && phys_address <= max_free_address)) {
		log_warn("0x%08lx is not in valid address range!", phys_address);
		return 0;
	}
	return 1;
}

/** @brief Returns number of free physical frames with physical address at
 *         least USER_MEM_START
 *
 *  @return Number of free physical frames
 */
uint32_t
num_free_phys_frames( void )
{
	return UNCLAIMED_PAGES + reuse_stack.top; //TODO why do we add this guy
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

    reuse_stack.top = 0;
    reuse_stack.len = PAGE_SIZE / sizeof(uint32_t);
    reuse_stack.data = smalloc(PAGE_SIZE);

    /* Crash kernel if we can't initialize phys frame allocator */
    affirm(reuse_stack.data);

	/* USER_MEM_START is the system wide 0 frame */
	max_free_address = USER_MEM_START + PAGE_SIZE;
	mutex_init(&mux);
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
	if (!physalloc_init)
		init_physalloc();

	mutex_lock(&mux);

	if (reuse_stack.top > 0) {
		mutex_unlock(&mux);
		return reuse_stack.data[--reuse_stack.top];
	}

	if (UNCLAIMED_PAGES == 0) {
		mutex_unlock(&mux);
		return 0; /* No more pages to allocate */
	}

	uint32_t frame = max_free_address;
	max_free_address += PAGE_SIZE;

	mutex_unlock(&mux);

	log_info("physalloc(): returned frame 0x%lx", frame);
	assert(is_physframe(frame));
	return frame;
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
	mutex_lock(&mux);
	affirm(is_physframe(phys_address));

	/* Add phys_address to stack, growing it if necessary. */
    if (reuse_stack.top >= reuse_stack.len) {
        assert(reuse_stack.top == reuse_stack.len);

        uint32_t *new_data = smalloc(reuse_stack.len * 2 * sizeof(uint32_t));
        if (!new_data) {
            log_warn("[ERROR] Losing free physical frames \
                    - no more kernel space.");
			mutex_unlock(&mux);
            return;
        }

        memcpy(new_data, reuse_stack.data, reuse_stack.len * sizeof(uint32_t));

        sfree(reuse_stack.data, reuse_stack.len * sizeof(uint32_t));
        reuse_stack.data = new_data;
        reuse_stack.len *= 2;
    }

    reuse_stack.data[reuse_stack.top++] = phys_address;
	mutex_unlock(&mux);
	log("physfree freed frame 0x%lx", phys_address);
}

/** @brief Tests physalloc and physfree
 *
 *  @return Void.
 */
void
test_physalloc( void )
{
	log_info("Testing physalloc(), physfree()");
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
	log("after all_phys");
	while (i < 1024) {
		all_phys[i] = physalloc();
		assert(all_phys[i]);
		i++;
		total--;
	}
	log("total frames supported:%08x",
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
		(void) addr;
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
	log("last frame start address:%lx", x);
	assert(!physalloc());

	/* put them all back */
	log("put all into linked list");
	while (total < TOTAL_USER_FRAMES) {
		total++;
		physfree(x);
		x -= PAGE_SIZE;
	}
	log_info("Tests passed!");
}

