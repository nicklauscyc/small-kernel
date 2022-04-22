/** @file 410user/progs/score.c
 *
 *  @author Ryan Chen (ryanc1)
 *  @author Urvi Agrawal (urvia)
 *
 *  @brief Runs batches of scoreboard tests based on flags provided in argv
 *  @public yes
 *  @for p3
 *  @covers argc/argv, fork(), exec(), new_pages(), remove_pages()
 *
 *  @bug None known
 */

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include "syscall.h"
#include "simics.h"
#include "string.h"

// These tests will be run if no other test suite is specified in argv.
// @covers Loading, getpid(), exec(), fork(), print()
char *default_tests[] = {
  "getpid_test1",
  "loader_test1",
  "loader_test2",
  "exec_basic",
  "exec_nonexist",
  "fork_test1",
  "print_basic",
  NULL,
};

// Run these tests by adding "memory" to argv.
// @covers new_pages(), remove_pages(), page fault handler,
//         zeroed allocations/ZFOD
char *memory_tests[] = {
  "new_pages",
  "remove_pages_test1",
  "mem_permissions",
  NULL,
};

typedef struct {
  char *name;
  char **programs;
} TestSuite;

// Array mapping strings potentially in argv[1] to test tables
TestSuite suites[] = {
  {"memory", memory_tests},
  {NULL, default_tests},
};

// You know how in kindergarten you didn't actually sleep during
// naptime so you just counted the number of tiles on the ceiling?
// No? Just me?
#define FAKE_SLEEP_DELAY 8096
int num_tiles_gcc_please_I_want_to_live = 0;

static void spin_delay() {
  for (int i = 0; i < FAKE_SLEEP_DELAY; i++) {
    ++num_tiles_gcc_please_I_want_to_live;
  }
}

// Returns 0 if x == NULL, y == NULL
// Returns some unspecified non-zero value if only one of x or y is NULL
// Otherwise, returns strcmp(x, y)
int safe_strcmp(char *x, char *y) {
  if (x == NULL && y == NULL) {
    return 0;
  } else if (x == NULL) {
    return -42;
  } else if (y == NULL) {
    return -42;
  } else {
    return strcmp(x, y);
  }
}

// Mass fork()/exec() all the tests in the named test suite.
void fork_test_suite(TestSuite *test_suite) {
  if (test_suite->name == NULL) {
    lprintf("score: Running default test suite.");
  } else {
    lprintf("score: Running test suite '%s'", test_suite->name);
  }

  int program_num = 0;
  while (test_suite->programs[program_num] != NULL) {
    int pid = fork();
    char *testname = test_suite->programs[program_num];

    if (pid < 0) {
      lprintf("score: fork() failed while launching %s", testname);
      spin_delay();
    } else if (pid == 0) {
      char *args[2];
      args[0] = testname;
      args[1] = NULL;

      exec(testname, args);

      lprintf("score: exec(%s) failed..?", testname);
    } else {  // pid > 0
      ++program_num;
    }
  }
}

void run_suite(char *suite_name) {
  int num_suites_loaded = sizeof(suites) / sizeof(suites[0]);
  for (int i = 0; i < num_suites_loaded; i++) {
    if (safe_strcmp(suite_name, suites[i].name) == 0) {
      fork_test_suite(&suites[i]);
      return;
    }
  }

  lprintf("score: test suite '%s' not found.", suite_name);
}


int main(int argc, char **argv) {
  if (argc == 0) {  // Maybe argv is not correct?
    lprintf("score: argc == 0??");
  } else {
    run_suite(NULL);        // Always run the default suite.
    if (argc >= 2) {
      run_suite(argv[1]);   // Additionally, run a named test suite
    }
  }

  // Since this test is meant to be run instead of init (before the shell is
  // functional), this program must be persistent.
  int status = 0;
  while (wait(&status)) continue;

  lprintf("score: collected all tasks");
  return -42;  // Hopefully not.
}
