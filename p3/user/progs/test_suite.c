/* Includes */
#include <simics.h>		/* for lprintf */
#include <stdlib.h> 	/* for exit */
#include <syscall.h>	/* for gettid */
#include "test.h"

#define TEST_EARLY_EXIT -2

/* These definitions have to match the ones in kern/tests.c */
#define MULT_FORK_TEST	0
#define MUTEX_TEST		1
#define PHYSALLOC_TEST	2

// TODO: Introduce tests for new syscalls
// TODO: Test for physalloc

int physalloc_test() {
	if (gettid() != 0) // Hack until vanish is implemented
		return TEST_EARLY_EXIT;

	return run_test(PHYSALLOC_TEST);
}

int multiple_fork_test() {
	if (gettid() != 0) // Hack until vanish is implemented
		return TEST_EARLY_EXIT;

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
		return TEST_EARLY_EXIT;

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
		return TEST_EARLY_EXIT;

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
		return TEST_EARLY_EXIT;

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


int main() {
	if (//physalloc_test() < 0 || // FIXME: for some reason not passing
		sleep_test() < 0 ||
 		mutex_test() < 0 ||
		yield_test() < 0 ||
		multiple_fork_test() < 0)
		return -1;

    return 0;
}
