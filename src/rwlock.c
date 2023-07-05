#include<rwlock.h>

static pthread_mutex_t* get_rwlock_lock(rwlock* rwlock_p)
{
	if(rwlock_p->has_internal_lock)
		return &(rwlock_p->internal_lock);
	else
		return rwlock_p->external_lock;
}

void initialize_rwlock(rwlock* rwlock_p, pthread_mutex_t* external_lock)
{
	if(external_lock)
	{
		rwlock_p->has_internal_lock = 0;
		rwlock_p->external_lock = external_lock;
	}
	else
	{
		rwlock_p->has_internal_lock = 1;
		pthread_mutex_init(&(rwlock_p->internal_lock), NULL);
	}

	rwlock_p->readers_count = 0;
	rwlock_p->writers_count = 0;
	rwlock_p->upgraders_waiting_count = 0;
	rwlock_p->readers_waiting_count = 0;
	rwlock_p->writers_waiting_count = 0;

	pthread_cond_init(&(rwlock_p->read_wait), NULL);
	pthread_cond_init(&(rwlock_p->write_wait), NULL);
	pthread_cond_init(&(rwlock_p->upgrade_wait), NULL);
}

void deinitialize_rwlock(rwlock* rwlock_p)
{
	if(rwlock_p->has_internal_lock)
		pthread_mutex_destroy(&(rwlock_p->internal_lock));
	pthread_cond_destroy(&(rwlock_p->read_wait));
	pthread_cond_destroy(&(rwlock_p->write_wait));
	pthread_cond_destroy(&(rwlock_p->upgrade_wait));
}

int read_lock(rwlock* rwlock_p, int non_blocking, int prefering);
int write_lock(rwlock* rwlock_p, int non_blocking);

int downgrade_lock(rwlock* rwlock_p);
int upgrade_lock(rwlock* rwlock_p, int non_blocking);

int read_unlock(rwlock* rwlock_p);

int write_unlock(rwlock* rwlock_p)
{
	int res = 0;

	if(rwlock_p->has_internal_lock)
		pthread_mutex_lock(get_rwlock_lock(rwlock_p));

	// make sure that the resource is write locked
	if(rwlock_p->writers_count == 0)
		goto EXIT;

	// decrement the writers_count, releasing write lock
	rwlock_p->writers_count--;
	res = 1;

	// wake up any waiters, a writer will always prefer a writer to have the lock
	if(rwlock_p->writers_waiting_count > 0)
		pthread_cond_signal(&(rwlock_p->write_wait));
	else if(rwlock_p->readers_waiting_count > 0)
		pthread_cond_broadcast(&(rwlock_p->read_wait));

	EXIT:;
	if(rwlock_p->has_internal_lock)
		pthread_mutex_unlock(get_rwlock_lock(rwlock_p));

	return res;
}

int is_read_locked(rwlock* rwlock_p)
{
	if(rwlock_p->has_internal_lock)
		pthread_mutex_lock(get_rwlock_lock(rwlock_p));

	int res = (rwlock_p->readers_count > 0);

	if(rwlock_p->has_internal_lock)
		pthread_mutex_unlock(get_rwlock_lock(rwlock_p));

	return res;
}

int is_write_locked(rwlock* rwlock_p)
{
	if(rwlock_p->has_internal_lock)
		pthread_mutex_lock(get_rwlock_lock(rwlock_p));

	int res = (rwlock_p->writers_count > 0);

	if(rwlock_p->has_internal_lock)
		pthread_mutex_unlock(get_rwlock_lock(rwlock_p));

	return res;
}

int has_waiters(rwlock* rwlock_p)
{
	if(rwlock_p->has_internal_lock)
		pthread_mutex_lock(get_rwlock_lock(rwlock_p));

	int res = (rwlock_p->readers_waiting_count > 0) ||
				(rwlock_p->writers_waiting_count > 0) ||
				(rwlock_p->upgraders_waiting_count > 0);

	if(rwlock_p->has_internal_lock)
		pthread_mutex_unlock(get_rwlock_lock(rwlock_p));

	return res;
}

int is_referenced(rwlock* rwlock_p)
{
	if(rwlock_p->has_internal_lock)
		pthread_mutex_lock(get_rwlock_lock(rwlock_p));

	int res = (rwlock_p->readers_count > 0) ||
				(rwlock_p->writers_count > 0) ||
				(rwlock_p->readers_waiting_count > 0) ||
				(rwlock_p->writers_waiting_count > 0) ||
				(rwlock_p->upgraders_waiting_count > 0);

	if(rwlock_p->has_internal_lock)
		pthread_mutex_unlock(get_rwlock_lock(rwlock_p));

	return res;
}