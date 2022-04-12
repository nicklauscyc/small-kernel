/* Includes */
#include <simics.h>		/* for lprintf */
#include <stdlib.h> 	/* for exit */
#include <syscall.h>	/* for gettid */
#include <string.h>		/* strncmp */
#include "test.h"
#include <assert.h>

#define TEST_EARLY_EXIT -2

/* These definitions have to match the ones in kern/tests.c */
#define MULT_FORK_TEST	0
#define MUTEX_TEST		1
#define PHYSALLOC_TEST	2
#define PD_CONSISTENCY  3

// TODO: Introduce tests for new syscalls


int physalloc_test() {
	if (gettid() != 0) // Hack until vanish is implemented
		return TEST_EARLY_EXIT;

	return run_test(PHYSALLOC_TEST);
}

/* This shouldn't be used in the test suite! */
void halt_test() {
	halt();

	panic("FAIL, halt_test");
}

int readfile_test() {
	if (gettid() != 0) // Hack until vanish is implemented
		return TEST_EARLY_EXIT;

	lprintf("Running readfile test");
	/* Read hello, then read world */
	char buf1[5]; // <- Hello
	char buf2[6]; // <- world!

	readfile("readfile_test_data", buf1, 5, 0);
	readfile("readfile_test_data", buf2, 6, 6);

	if (strncmp(buf1, "Hello", 5) || strncmp(buf2, "world!", 6)) {
		lprintf("FAIL, readfile_test");
		return -1;
	}

	if (gettid() != 0) // Hack until vanish is implemented
		return TEST_EARLY_EXIT;
	lprintf("SUCCESS, readfile_test");
	return 0;
}

void pagefault_test() {
	lprintf("Writing into read-only address - should crash.");
	void *a = readfile_test;
	*(int *)a = 1;
}


int print_test() {
	if (gettid() != 0) // Hack until vanish is implemented
		return TEST_EARLY_EXIT;

	lprintf("Running print test. Equal characters should not be interleaved");

    int pid1 = fork();
    int pid2 = fork();

	char A[] = "Aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
			   "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaA";

	char B[] = "Bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
			   "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbB";

	char C[] = "Ccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
			   "ccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccC";

	char D[] = "Ddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd"
			   "ddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddD";

	if (!pid1 && !pid2)
		print(sizeof(A)/sizeof(char), A);

	if (pid1 && !pid2)
		print(sizeof(B)/sizeof(char), B);

	if (!pid1 && pid2)
		print(sizeof(C)/sizeof(char), C);

	if (pid1 && pid2)
		print(sizeof(D)/sizeof(char), D);

	if (gettid() != 0) // Hack until vanish is implemented
		return TEST_EARLY_EXIT;
	return 0;
}

int cursor_test() {
	lprintf("Running cursor_test");
	set_cursor_pos(0,0);
	int row;
	int col;
	get_cursor_pos(&row, &col);

	if (row || col) {
		lprintf("FAIL, cursor_test");
		return -1;
	}

	if (gettid() != 0) // Hack until vanish is implemented
		return TEST_EARLY_EXIT;
	lprintf("SUCCESS, cursor_test");
	return 0;
}

int multiple_fork_test() {
    int pid1 = fork();
    int pid2 = fork();

    (void)pid1;
    (void)pid2;

    int result = run_test(MULT_FORK_TEST);

    /* Cleanup after ourselves, TODO: enable after implementing vanish */
    //if (pid1 || pid2)
    //    vanish();


	if (gettid() != 0) // Hack until vanish is implemented
		return TEST_EARLY_EXIT;

    return result;
}

int mutex_test() {
    int pid = fork();

    (void)pid;

    int result = run_test(MUTEX_TEST);

    /* Cleanup after ourselves, TODO: enable after implementing vanish */
    //if (pid)
    //    vanish();

	if (gettid() != 0) // Hack until vanish is implemented
		return TEST_EARLY_EXIT;
    return result;
}

int sleep_test() {
	/* Need to fork or we get deadlock, currently no idle task. */

	int ticks = 10000;
	lprintf("Running sleep_test, sleeping for %d seconds (%d ticks).\
			 Currently at tick %d", ticks / 1000, ticks, get_ticks());

	if (fork()) {
		sleep(ticks);
		lprintf("Passed sleep_test. Now at tick %d", get_ticks());
	}

	if (gettid() != 0) // Hack until vanish is implemented
		return TEST_EARLY_EXIT;

	return 0;
}

int yield_test() {
	lprintf("Running yield_test");

    int pid = fork();

	if (!pid)
		return -1;

	if (yield(pid) != 0 || yield(-1) != 0 || yield(786234) == 0) {
		lprintf("FAILURE, yield_test");
		return -1;
	}

    /* Cleanup after ourselves, TODO: enable after implementing vanish */
    //if (pid)
    //    vanish();

	if (gettid() != 0) // Hack until vanish is implemented
		return TEST_EARLY_EXIT;
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
	lprintf("name is %s=='a'", name);
	res = remove_pages(name);
	assert(res == 0);
	lprintf("new_pages test passed");
	return 0;
}

int
pd_test( void )
{
    return run_test(PD_CONSISTENCY);
}

int remove_pages_test( void )
{
	assert(remove_pages((int *) 0x4) < 0);
	assert(remove_pages((int *) 0xdeadd00d) < 0);
	assert(remove_pages((int *) 0xffffe000) < 0);
	assert(remove_pages((int *) 0xfffff000) < 0);
	lprintf("remove_pages_test() passed!");
	return 0;
}
int main() {
	//pagefault_test();

	// physalloc_test() works only during startup, will fail here, TODO: fix it
	if (remove_pages_test() < 0 ||
		pd_test() < 0 ||
        new_pages_test() < 0 ||
		readfile_test() < 0 ||
		cursor_test() < 0 ||
		print_test() < 0 ||
		sleep_test() < 0 ||
 		mutex_test() < 0 ||
		yield_test() < 0 ||
		multiple_fork_test() < 0)
		return -1;

	lprintf("ALL TESTS PASSED!");
    return 0;
}
