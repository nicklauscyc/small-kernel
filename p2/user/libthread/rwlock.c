

#include <rwlock_type.h> /* rwlock_t */

/* readers/writers lock functions */
int rwlock_init( rwlock_t *rwlock )
{
	return 0;
}
void rwlock_lock( rwlock_t *rwlock, int type )
{
	(void) rwlock;
	(void) type;
	return;
}

void rwlock_unlock( rwlock_t *rwlock )
{
	return;
}

void rwlock_destroy( rwlock_t *rwlock )
{
	return;
}

void rwlock_downgrade( rwlock_t *rwlock)
{
	return;
}


