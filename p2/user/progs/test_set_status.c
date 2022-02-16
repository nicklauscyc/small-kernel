#include <syscall.h>
#include <stdlib.h> /* exit() */
#include <simics.h> /* lprintf */
#include <mutex.h> /* mutex */
#include <thr_internals.h> /* add_one_atomic(), thread_fork() */
#include <assert.h>
#include <mutex.h>

void test_exec();
void test_fork_and_wait();
void test_thread_management();
void test_mutex();
void test_add_one_atomic();

int main() {
    test_mutex();
	exit(69);
}

void test_add_one_atomic() {
    uint32_t result = 0;
    int ticket;

    for (int i = 0; i < 100; i++) {
        ticket = add_one_atomic(&result);
		ticket++;
    }

    assert(result == 100);
}

// At dad should be printed before At son
void test_mutex() {
    mutex_t m;
    assert(mutex_init(&m) >= 0);
    mutex_lock(&m);
    sleep(500);
    mutex_unlock(&m);

    mutex_destroy(&m);
}

void test_thread_management() {
    int tid;
	mutex_t *mp = malloc(sizeof(mp));
	mutex_init(mp);

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
