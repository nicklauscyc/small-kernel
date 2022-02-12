#include <syscall.h>
#include <stdlib.h> /* exit() */
#include <simics.h> /* lprintf */
#include <assert.h>

void test_exec();
void test_fork_and_wait();
void test_thread_management();

int main() {
	exit(69);
}

void test_thread_management() {
    int tid;
    if ((tid = fork())) {
        lprintf("Child tid is %d", tid);
        sleep(100);
        assert(yield(tid) <= 0); /* Ensure child is descheduled */
        make_runnable(tid);
    } else {
        int reject = 0;
        deschedule(&reject);
        lprintf("My tid is %d", gettid());
    }
}

/* Should run cat and return error and different exit status */
void test_exec() {
    char *argvec[] = { "cat", ".", NULL };
    exec(argvec[0], argvec);
}

void test_fork_and_wait() {
    char hello_dad[] = "Hello from parent\n";
    char hello_son[] = "Hello from child\n";

    int tid;
    if ((tid = fork())) {
        print(sizeof(hello_dad), hello_dad);

        assert(tid == wait(NULL));
    } else {
        print(sizeof(hello_son), hello_son);
    }

}
