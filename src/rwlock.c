#include<rwlock.h>

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
	rwlock_p->writer_count = 0;
	rwlock_p->upgraders_waiting_count = 0;
	rwlock_p->readers_waiting_count = 0;
	rwlock_p->writers_waiting_count = 0;

	pthread_cond_init(&(rwlock_p->read_wait), NULL);
	pthread_cond_init(&(rwlock_p->write_wait), NULL);
	pthread_cond_init(&(rwlock_p->upgrade_wait), NULL);
}

void deinitialize_rwlock(rwlock* rwlock_p);

int read_lock(rwlock* rwlock_p, int non_blocking, int prefering);
int write_lock(rwlock* rwlock_p, int non_blocking);

int downgrade_lock(rwlock* rwlock_p);
int upgrade_lock(rwlock* rwlock_p, int non_blocking);

int read_unlock(rwlock* rwlock_p);
int write_unlock(rwlock* rwlock_p);

int is_read_locked(rwlock* rwlock_p);
int is_write_locked(rwlock* rwlock_p);
int has_waiters(rwlock* rwlock_p);
int is_referenced(rwlock* rwlock_p);