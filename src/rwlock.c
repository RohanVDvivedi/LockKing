#include<rwlock/rwlock.h>

#include<posixutils/pthread_cond_utils.h>

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

	pthread_cond_init_with_monotonic_clock(&(rwlock_p->read_wait));
	pthread_cond_init_with_monotonic_clock(&(rwlock_p->write_wait));
	pthread_cond_init_with_monotonic_clock(&(rwlock_p->upgrade_wait));
}

void deinitialize_rwlock(rwlock* rwlock_p)
{
	if(rwlock_p->has_internal_lock)
		pthread_mutex_destroy(&(rwlock_p->internal_lock));
	pthread_cond_destroy(&(rwlock_p->read_wait));
	pthread_cond_destroy(&(rwlock_p->write_wait));
	pthread_cond_destroy(&(rwlock_p->upgrade_wait));
}

static inline int can_grab_read_lock(const rwlock* rwlock_p, lock_preferring_type preferring)
{
	if(preferring == READ_PREFERRING) // in read preferring mode, you grab lock immediately when you see that no writers hold lock
		return (rwlock_p->writers_count == 0);
	else // while in write preferring mode, it favors writers over readers, and waiting for all waiters trying to hold write lock to exit
		return (rwlock_p->writers_count == 0) && (rwlock_p->writers_waiting_count == 0) && (rwlock_p->upgraders_waiting_count == 0);
}

int read_lock(rwlock* rwlock_p, lock_preferring_type preferring, uint64_t timeout_in_microseconds)
{
	int res = 0;

	if(rwlock_p->has_internal_lock)
		pthread_mutex_lock(get_rwlock_lock(rwlock_p));

	if(timeout_in_microseconds != NON_BLOCKING) // you are allowed to block only if (timeout_in_microseconds != NON_BLOCKING)
	{
		int wait_error = 0;
		while(!can_grab_read_lock(rwlock_p, preferring) && !wait_error) // block while you can not grab lock and there is no wait error
		{
			rwlock_p->readers_waiting_count++;
			if(timeout_in_microseconds == BLOCKING)
				wait_error = pthread_cond_wait(&(rwlock_p->read_wait), get_rwlock_lock(rwlock_p));
			else
				wait_error = pthread_cond_timedwait_for_microseconds(&(rwlock_p->read_wait), get_rwlock_lock(rwlock_p), &timeout_in_microseconds);
			rwlock_p->readers_waiting_count--;
		}
	}

	// if you can grab a lock, then grab it, else fail
	if(can_grab_read_lock(rwlock_p, preferring))
	{
		rwlock_p->readers_count++;
		res = 1;
	}

	if(rwlock_p->has_internal_lock)
		pthread_mutex_unlock(get_rwlock_lock(rwlock_p));

	return res;
}

static inline int can_grab_write_lock(const rwlock* rwlock_p)
{
	// a write lock can only be grabbed if there are no active readers and writers
	return (rwlock_p->readers_count == 0) && (rwlock_p->writers_count == 0);
}

int write_lock(rwlock* rwlock_p, uint64_t timeout_in_microseconds)
{
	int res = 0;

	if(rwlock_p->has_internal_lock)
		pthread_mutex_lock(get_rwlock_lock(rwlock_p));

	if(non_blocking)
	{
		// we can non blockingly take the write lock, only if there are no readers or writers accessing the resource
		if(rwlock_p->readers_count == 0 && rwlock_p->writers_count == 0)
		{
			rwlock_p->writers_count++;
			res = 1;
		}
	}
	else
	{
		// block until there are any readers or writers accessing the resource
		while(rwlock_p->readers_count > 0 || rwlock_p->writers_count > 0)
		{
			rwlock_p->writers_waiting_count++;
			pthread_cond_wait(&(rwlock_p->write_wait), get_rwlock_lock(rwlock_p));
			rwlock_p->writers_waiting_count--;
		}

		rwlock_p->writers_count++;
		res = 1;
	}

	if(rwlock_p->has_internal_lock)
		pthread_mutex_unlock(get_rwlock_lock(rwlock_p));

	return res;
}

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

// before calling this unction you need to ensure that you are actually holding a reader lock, and that there are no upgraders waiting
// i.e. writers_count == 0 && readers_count == 1
static inline int can_upgrade_lock(const rwlock* rwlock_p)
{
	// you can go ahead with upgrading the reader lock held into a writer lock, only if we are the sole person holding the reader lock
	return (rwlock_p->readers_count == 1);
}

int upgrade_lock(rwlock* rwlock_p, uint64_t timeout_in_microseconds)
{
	int res = 0;

	if(rwlock_p->has_internal_lock)
		pthread_mutex_lock(get_rwlock_lock(rwlock_p));

	// you can not be holding a reader lock (assumed since you want to upgrade), if there are active writers
	if(rwlock_p->writers_count > 0)
		goto EXIT;

	// there are not read locks issued, so you can not be holding a reader lock
	if(rwlock_p->readers_count == 0)
		goto EXIT;

	// we can not even wait to upgrade the lock, if there is someone else aswell wanting to upgrade the lock
	if(rwlock_p->upgraders_waiting_count > 0)
		goto EXIT;

	if(non_blocking)
	{
		// to non blockingly take the lock,
		// there must not be any other reader in the system, other than us
		if(rwlock_p->readers_count == 1)
		{
			rwlock_p->readers_count--;
			rwlock_p->writers_count++;
			res = 1;
		}
	}
	else
	{
		// only a reader thread (thread with read lock on the resource) can go on to wait for an upgrade to a writer lock
		// hence rwlock_p->upgraders_waiting_count <= rwlock_p->readers_count
		while(rwlock_p->readers_count > 1)
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
	// by default logic rwlock_p->readers_count >= rwlock_p->upgraders_waiting_count
	// if they are equal then all the readers are waiting for an upgrade and hence couldn't have requested a read_unlock
	if(rwlock_p->readers_count == rwlock_p->upgraders_waiting_count)
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