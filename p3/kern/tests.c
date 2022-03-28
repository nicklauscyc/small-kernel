/** @file tests.c
 *  Set of tests for user/kern interactions */

#include <tests.h>
#include <assert.h>
#include <lib_thread_management/mutex.h>
#include <interrupt_defines.h> /* INT_CTL_PORT, INT_ACK_CURRENT */
#include <asm.h> /* outb() */
#include <simics.h>
#include <logger.h>

#define MULT_FORK_TEST 0
#define MUTEX_TEST 1

static volatile int total_sum = 0;
static mutex_t mux;

/* Init is run once, before any syscalls can be made */
void
init_tests( void )
{
    mutex_init(&mux);
}

int
mult_fork_test()
{
    log_info("Running mult_fork_test");

    /* Give threads enough time to context switch */
    for (int i=0; i < 1 << 24; ++i)
        total_sum++;

    log_info("SUCCESS, mult_fork_test");
    return 0;
}

int
mutex_test()
{
    log_info("Running mutex_test");

    mutex_lock(&mux);
    int old_total_sum = total_sum;
    for (int i=0; i < 1 << 24; ++i)
        total_sum++;
    if (total_sum != old_total_sum + (1 << 24)) {
        log_info("FAIL, mutex_test");
        return -1;
    }
    mutex_unlock(&mux);

    log_info("SUCCESS, mutex_test");
    return 0;
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
	int res = install_handler_in_idt(idt_entry, asm_wrapper, DPL_3);
	return res;
}

