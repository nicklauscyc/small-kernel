#include <tests.h>
#include <assert.h>

static volatile int total_sum = 0;
static mutex_t mux;

/* Init is run once, before any syscalls can be made */
void
init_tests( void )
{
    mutex_init(&mux);
}

int
test_int_handler( int test_num )
{
    /* Acknowledge interrupt and return */
    outb(INT_CTL_PORT, INT_ACK_CURRENT);

    mutex_lock(&mux);
    int old_total_sum = total_sum;
    for (int i=0; i < 1 << 24; ++i)
        total_sum++;
    affirm(total_sum == old_total_sum + (1 << 24));
    mutex_unlock(&mux);

    return 0;
}

/** @brief Installs the test() interrupt handler
 */
int
install_test_handler( uint32_t idt_entry, asm_wrapper_t *asm_wrapper )
{
	if (!asm_wrapper) {
		return -1;
	}
	init_tests();
	int res = install_handler_in_idt(idt_entry, asm_wrapper, DPL_3);
	return res;
}

