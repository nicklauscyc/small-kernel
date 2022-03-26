/* Includes */
#include <simics.h>  /* for lprintf */
#include <stdlib.h>  /* for exit */
#include <syscall.h> /* for gettid */
#include "test.h"

/* Main */
int main() {

    fork();

    run_test(0);
}
