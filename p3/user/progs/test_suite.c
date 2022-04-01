/* Includes */
#include <simics.h>  /* for lprintf */
#include <stdlib.h>  /* for exit */
#include <syscall.h> /* for gettid */
#include "test.h"

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

int yield_test() {
	if (gettid() != 0) // Hack until vanish is implemented
		return 0;
	lprintf("Running yield_test");

    int pid = fork();

	if (yield(pid) != 0 || yield(-1) != 0 || yield(pid + 10) == 0) {
		lprintf("FAILURE, yield_test");
		return -1;
	}

	lprintf("SUCCESS, yield_test");
	return 0;

    //int result = run_test(YIELD_TEST);

    /* Cleanup after ourselves, TODO: enable after implementing vanish */
    //if (pid)
    //    vanish();

    //return result;
}


int main() {
    if (mutex_test() < 0 ||
		yield_test() < 0 ||
        multiple_fork_test() < 0)
        return -1;

    return 0;
}
