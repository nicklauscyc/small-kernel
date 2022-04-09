/* Includes */
#include <simics.h>  /* for lprintf */
#include <stdlib.h>  /* for exit */
#include <syscall.h> /* for gettid */
#include "test.h"
#include <assert.h>

#define MULT_FORK_TEST 0
#define MUTEX_TEST 1
#define YIELD_TEST 2

int multiple_fork_test() {
	if (gettid() != 0) // Hack until vanish is implemented
		return 0;

    int pid1 = fork();
    int pid2 = fork();

    (void)pid1;
    (void)pid2;

    int result = run_test(MULT_FORK_TEST);

    /* Cleanup after ourselves, TODO: enable after implementing vanish */
    //if (pid1 || pid2)
    //    vanish();

    return result;
}

int mutex_test() {
	if (gettid() != 0) // Hack until vanish is implemented
		return 0;

    int pid = fork();

    (void)pid;

    int result = run_test(MUTEX_TEST);

    /* Cleanup after ourselves, TODO: enable after implementing vanish */
    //if (pid)
    //    vanish();

    return result;
}

int sleep_test() {
	if (gettid() != 0) // Hack until vanish is implemented
		return 0;

	/* Need to fork or we get deadlock, currently no idle task. */

	int ticks = 10000;
	lprintf("Running sleep_test, sleeping for %d seconds (%d ticks).\
			 Currently at tick %d", ticks / 1000, ticks, get_ticks());

	if (fork()) {
		sleep(ticks);
		lprintf("Passed sleep_test. Now at tick %d", get_ticks());
	}

	return 0;
}

int yield_test() {
	if (gettid() != 0) // Hack until vanish is implemented
		return 0;
	lprintf("Running yield_test");

    int pid = fork();

	if (pid)
		return 0;

	if (yield(pid) != 0 || yield(-1) != 0 || yield(pid + 10) == 0) {
		lprintf("FAILURE, yield_test");
		return -1;
	}

    /* Cleanup after ourselves, TODO: enable after implementing vanish */
    //if (pid)
    //    vanish();

	lprintf("SUCCESS, yield_test");
	return 0;
}

int new_pages_test(){

	char *name = (char *) (0x1000000 + 7*PAGE_SIZE);
	int len = 2 * PAGE_SIZE;
	int res = new_pages(name, len);
	assert(res == 0);
	assert(new_pages(name, len) < 0);
	lprintf("new_pages allocated");

	/* Test writing to name */
	char c = *name;
	lprintf("c:%c", c); // reading this works
	char a = 'a';
	lprintf("a address:%p", &a);
	*name = a;
	lprintf("new_pages test passed");
	return 0;
}

int main() {
    if (new_pages_test() < 0 ||
		sleep_test() < 0 ||
		mutex_test() < 0 ||
		yield_test() < 0 ||
        multiple_fork_test() < 0
		)
		return -1;

	lprintf("ALL TESTS PASSED!");
    return 0;
}
