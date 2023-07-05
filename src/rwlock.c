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

int downgrade_lock(rwlock* rwlock_p)
{
	int res = 0;

	if(rwlock_p->has_internal_lock)
		pthread_mutex_lock(get_rwlock_lock(rwlock_p));

	// make sure that the resource is write locked
	if(rwlock_p->writers_count == 0)
		goto EXIT;

	// decrement the writers_count, increment readers_count, releasing converting a read lock to a write lock
	rwlock_p->writers_count--;
	rwlock_p->readers_count++;
	res = 1;

	// since before this call I was a writer, there can not be any upgraders waiting in the system

	// so we only need to wake up readers
	if(rwlock_p->readers_waiting_count > 0)
		pthread_cond_broadcast(&(rwlock_p->read_wait));

	EXIT:;
	if(rwlock_p->has_internal_lock)
		pthread_mutex_unlock(get_rwlock_lock(rwlock_p));

	return res;
}

int upgrade_lock(rwlock* rwlock_p, int non_blocking)
{
	int res = 0;

	if(rwlock_p->has_internal_lock)
		pthread_mutex_lock(get_rwlock_lock(rwlock_p));

	// make sure that the resource is read locked
	if(rwlock_p->readers_count == 0)
		goto EXIT;

	if(non_blocking)
	{
		// to non blockingly take the lock,
		// there must not be any other reader in the system
		// and there should not be any upgraders in the system
		if(rwlock_p->readers_count == 1 && rwlock_p->upgraders_waiting_count == 0)
		{
			rwlock_p->readers_count--;
			rwlock_p->writers_count++;
			res = 1;
		}
	}
	else
	{
		// we can not even wait to upgrade the lock, if there is someone else aswell wanting to upgrade the lock
		if(rwlock_p->upgraders_waiting_count > 0)
			goto EXIT;

		while(rwlock_p->readers_count != 1)
		{
			rwlock_p->upgraders_waiting_count++;
			pthread_cond_wait(&(rwlock_p->upgrade_wait), get_rwlock_lock(rwlock_p));
			rwlock_p->upgraders_waiting_count--;
		}

		// once the readers count has reached 1, we upgrade the lock
		rwlock_p->readers_count--;
		rwlock_p->writers_count++;
		res = 1;
	}

	EXIT:;
	if(rwlock_p->has_internal_lock)
		pthread_mutex_unlock(get_rwlock_lock(rwlock_p));

	return res;
}

int read_unlock(rwlock* rwlock_p)
{
	int res = 0;

	if(rwlock_p->has_internal_lock)
		pthread_mutex_lock(get_rwlock_lock(rwlock_p));

	// make sure that the resource is read locked
	if(rwlock_p->readers_count == 0)
		goto EXIT;

	// decrement the readers_count, releasing read lock
	rwlock_p->readers_count--;
	res = 1;

	// wake up any waiters (upgraders, writers or any possible waiting readers), only if this is the last reader thread
	if(rwlock_p->readers_count == 1 && rwlock_p->upgraders_waiting_count > 0)
		pthread_cond_signal(&(rwlock_p->upgrade_wait));
	else if(rwlock_p->readers_count == 0 && rwlock_p->writers_waiting_count > 0)
		pthread_cond_signal(&(rwlock_p->write_wait));
	else if(rwlock_p->readers_count == 0 && rwlock_p->readers_waiting_count > 0) // this is redundant, since readers will never wait if there are no writers or upgraders waiting
		pthread_cond_broadcast(&(rwlock_p->read_wait));

	EXIT:;
	if(rwlock_p->has_internal_lock)
		pthread_mutex_unlock(get_rwlock_lock(rwlock_p));

	return res;
}

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