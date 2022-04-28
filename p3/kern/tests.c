/** @file tests.c
 *  Set of tests for user/kern interactions */

#include <tests.h>

#include <asm.h>		/* outb() */
#include <assert.h>
#include <simics.h>
#include <logger.h>
#include <physalloc.h>	/* physalloc_test() */
#include <interrupt_defines.h>	/* INT_CTL_PORT, INT_ACK_CURRENT */
#include <lib_thread_management/mutex.h>
#include <memory_manager.h>
#include <x86/cr.h>
#include <x86/page.h>
#include <common_kern.h>    /* USER_MEM_START */

/* These definitions have to match the ones in user/progs/test_suite.c */
#define MULT_FORK_TEST	0
#define MUTEX_TEST		1
#define PHYSALLOC_TEST	2
#define PD_CONSISTENCY  3
#define TOTAL_USER_FRAMES (machine_phys_frames() - (USER_MEM_START / PAGE_SIZE))

static volatile int total_sum_fork = 0;
static volatile int total_sum_mux = 0;
static mutex_t mux;

/* Init is run once, before any syscalls can be made */
void
init_tests( void )
{
    mutex_init(&mux);
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
	int total = (machine_phys_frames() - (USER_MEM_START / PAGE_SIZE));
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

int
mult_fork_test()
{
	log_info("Running mult_fork_test");

    /* Give threads enough time to context switch */
    for (int i=0; i < 1 << 24; ++i)
        total_sum_fork++;

    log_info("SUCCESS, mult_fork_test");
    return 0;
}

int
mutex_test()
{
    log_info("Running mutex_test");

    mutex_lock(&mux);
    int old_total_sum = total_sum_mux;
    for (int i=0; i < 1 << 24; ++i)
        total_sum_mux++;
    if (total_sum_mux != old_total_sum + (1 << 24)) {
        log_info("FAIL, mutex_test.");
        return -1;
    }
    mutex_unlock(&mux);

    log_info("SUCCESS, mutex_test");
    return 0;
}

void
test_pd_consistency( void )
{
    lprintf("testing pd_consistency");
	affirm(is_valid_pd((void *)TABLE_ADDRESS(get_cr3())));
}


int
test_int_handler( int test_num )
{
    /* Acknowledge interrupt and return */
    outb(INT_CTL_PORT, INT_ACK_CURRENT);

    switch (test_num) {
        case MULT_FORK_TEST:
            return mult_fork_test();
            break;
        case MUTEX_TEST:
            return mutex_test();
            break;
		case PHYSALLOC_TEST:
			test_physalloc();
			return 0;
        case PD_CONSISTENCY:
            test_pd_consistency();
            return 0;
    }

    return 0;
}

/** @brief Installs the test() interrupt handler
 */
int
install_test_handler( int idt_entry, asm_wrapper_t *asm_wrapper )
{
	if (!asm_wrapper) {
		return -1;
	}
	init_tests();
	int res = install_handler_in_idt(idt_entry, asm_wrapper, DPL_3, D32_TRAP);
	return res;
}

