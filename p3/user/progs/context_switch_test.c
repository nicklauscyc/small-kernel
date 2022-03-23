/* Includes */
#include <simics.h>  /* for lprintf */
#include <stdlib.h>  /* for exit */
#include <syscall.h> /* for getpid */

/* Main */
int main() {

    fork();

    int tid = gettid();

    for (int i=0; i < 10; ++i)
        lprintf("expect 1: %d", gettid() == tid);
}
