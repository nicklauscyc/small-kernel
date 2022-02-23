#include <thread.h>
#include <mutex.h>
#include <cond.h>
#include <rwlock.h>
#include <syscall.h>
#include <stdlib.h>
#include <stdio.h>
#include <simics.h>
#include <assert.h>

#define STACK_SIZE 4096

int read_count = 0;
mutex_t read_count_lock;

rwlock_t lock;

void *h(void *arg)
{
	rwlock_lock(&lock, RWLOCK_WRITE);
    *(int *)arg = 1;
    rwlock_unlock(&lock);
	return((void *)0);
}

void *f(void *arg)
{
	rwlock_lock(&lock, RWLOCK_READ);
    assert(*(int *)arg == 0);
	rwlock_unlock(&lock);

	return((void *)0);
}

void *g(void *arg)
{
	rwlock_lock(&lock, RWLOCK_READ);
    assert(*(int *)arg == 1);
	rwlock_unlock(&lock);

	return((void *)0);
}

int main()
{
	thr_init(STACK_SIZE);

	rwlock_init(&lock);
	//mutex_init(&read_count_lock);
	rwlock_lock(&lock, RWLOCK_WRITE);

    int i = 0;

    /* Write 1, should fail */
    if (thr_create(h, &i) < 0) {
        return -1;
    }

    for (int j=0; j < 10; ++j) {
        if (thr_create(f, &i) < 0) {
            return -1;
        }
    }

    sleep(100); /* Wait for everyone to join the queue. */

	rwlock_downgrade(&lock);

    sleep(100); /* Everyone reading and then someone writes */

    if (thr_create(g, &i) < 0) {  /* Ensure writer has had access */
        return -1;
    }

	return 0;
}
