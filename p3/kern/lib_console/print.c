/** @file print.c
 *  @brief Print syscall handler and facilities for managing
 *		   concurrent prints
 */
#include <asm.h>				/* outb */
#include <assert.h>				/* affirm */
#include <console.h>			/* putbytes */
#include <memory_manager.h>		/* is_valid_user_pointer */
#include <interrupt_defines.h>	/* INT_CTL_PORT, INT_ACK_CURRENT */
#include <lib_thread_management/mutex.h>	/* mutex_t */

/** @brief Mutex for print calls */
static mutex_t print_mux;

/** @brief Whether print is initialized */
static int print_initialized = 0;

/** @brief Initializes print module
 *
 *  @return Void. */
static void
init_print( void )
{
	affirm(!print_initialized);
	mutex_init(&print_mux);
	print_initialized = 1;
}

/** @brief Handler for print syscall.
 *
 *  @param len Length of message to print
 *  @param buf Buffer to read from
 *  @return 0 on success, negative value on error */
int
print( int len, char *buf )
{
	/* Check init before acknowledging interrupt to ensure
	 * no race conditions among concurrent init_print() calls */
	if (!print_initialized)
		init_print();

    /* Acknowledge interrupt */
    outb(INT_CTL_PORT, INT_ACK_CURRENT);

	if (!is_valid_user_string(buf, len))
		return -1;

	mutex_lock(&print_mux);

	putbytes(buf, len);

	mutex_unlock(&print_mux);

	return 0;
}
