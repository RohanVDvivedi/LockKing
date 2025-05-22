#ifndef RW_LOCK_H
#define RW_LOCK_H

#include<pthread.h>
#include<stdint.h>

#include<lockking/lockking_commons.h>

// rwlock assumes that the thread count in your application will never be more than UINT64_MAX

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

typedef enum lock_preferring_type lock_preferring_type;
enum lock_preferring_type
{
	READ_PREFERRING,
	WRITE_PREFERRING,
};

// *_lock and upgrade lock functions may fail if non_blocking = 1 and the lock can not be immediatley taken
int read_lock(rwlock* rwlock_p, lock_preferring_type preferring, uint64_t timeout_in_microseconds);
int write_lock(rwlock* rwlock_p, uint64_t timeout_in_microseconds);

// upgrades lock from reader to a writer
// this may fail if there already is a reader thread waiting for an upgrade
int upgrade_lock(rwlock* rwlock_p, uint64_t timeout_in_microseconds);

// *_unlock and downgrade function never blocks

// downgrades lock from a writer to a reader
int downgrade_lock(rwlock* rwlock_p);

int read_unlock(rwlock* rwlock_p);
int write_unlock(rwlock* rwlock_p);

// use the below 4 functions only with an external_lock held, else they give only instantaneous results

int is_read_locked(rwlock* rwlock_p);
int is_write_locked(rwlock* rwlock_p);
int has_rwlock_waiters(rwlock* rwlock_p);
int is_rwlock_referenced(rwlock* rwlock_p);

/*
	to define an api for shared lock and exclusive lock based nomenclature
	the below aliases have been defined
*/

#define shared_lock         read_lock
#define shared_unlock       read_unlock
#define exclusive_lock      write_lock
#define exclusive_unlock    write_unlock
#define is_shared_locked    is_read_locked
#define is_exclusive_locked is_write_locked

#endif