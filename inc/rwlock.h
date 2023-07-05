#ifndef RW_LOCK_H
#define RW_LOCK_H

#include<pthread.h>
#include<stdint.h>

typedef struct rwlock rwlock;
struct rwlock
{
	int has_internal_lock : 1;
	unsigned int writers_count : 1;
	unsigned int upgraders_waiting_count : 1;

	uint64_t readers_count;
	uint64_t readers_waiting_count;
	uint64_t writers_waiting_count;

	union{
		pthread_mutex_t internal_lock;
		pthread_mutex_t* external_lock;
	};

	pthread_cond_t read_wait; // readers wait here
	pthread_cond_t write_wait; // writers wait here
	pthread_cond_t upgrade_wait; // upgrader waits here
};

void initialize_rwlock(rwlock* rwlock_p, pthread_mutex_t* external_lock);
void deinitialize_rwlock(rwlock* rwlock_p);

// majorly the api only has below 6 functions

#define READ_PREFERING 0
#define WRITE_PREFERING 1

// *_lock and upgrade lock functions may fail if non_blocking = 1 and the lock can not be immediatley taken
int read_lock(rwlock* rwlock_p, int non_blocking, int prefering);
int write_lock(rwlock* rwlock_p, int non_blocking);

// upgrades lock from reader to a writer
// this may fail if there already is a reader thread waiting for an upgrade
int upgrade_lock(rwlock* rwlock_p, int non_blocking);

// *_unlock and downgrade function never blocks

// downgrades lock from a writer to a reader
int downgrade_lock(rwlock* rwlock_p);

int read_unlock(rwlock* rwlock_p);
int write_unlock(rwlock* rwlock_p);

// use the below 4 functions only with an external lock held, else they give only instantaneous results

int is_read_locked(rwlock* rwlock_p);
int is_write_locked(rwlock* rwlock_p);
int has_waiters(rwlock* rwlock_p);
int is_referenced(rwlock* rwlock_p);

#endif