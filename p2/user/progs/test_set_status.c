#include <syscall.h>
#include <stdlib.h> /* exit() */
#include <simics.h> /* lprintf */
#include <mutex.h> /* mutex */
#include <thr_internals.h> /* add_one_atomic() */
#include <assert.h>
#include <mutex.h>
#include <thread.h>

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

int val = 0;

void *conflict_a(void *arg) {
    lprintf("Hello from conflict_a");
    mutex_t *m = (mutex_t *)arg;
    for (int i=0; i<1000; ++i) {
        mutex_lock(m);
        for (int j=0; j<1000; ++j) {
            val++;
        }
        mutex_unlock(m);
    }
    lprintf("Conflict a done");
    return 0;
}

void *conflict_b(void *arg) {
    lprintf("Hello from conflict_b");
    mutex_t *m = (mutex_t *)arg;
    for (int i=0; i<1000; ++i) {
        mutex_lock(m);
        for (int j=0; j<1000; ++j) {
            val++;
        }
        mutex_unlock(m);
    }
    lprintf("Conflict b done");
    return 0;
}

void test_mutex() {
    thr_init(1024);
    mutex_t m;
    assert(mutex_init(&m) >= 0);

    thr_create(conflict_a, &m);
    thr_create(conflict_b, &m);
    // No join yet, so just sleep for a while
    sleep(1000);
    lprintf("val %d (expect 2000000)", val);
    assert(val = 2000000);

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
