#ifndef RW_LOCK_H
#define RW_LOCK_H

#include<pthread.h>
#include<stdint.h>

typedef struct rwlock rwlock;
struct rwlock
{
	int has_internal_lock : 1;
	unsigned int writer_count : 1;
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
};

void initialize_rwlock(rwlock* rwlock_p, pthread_mutex_t* external_lock);
int deinitialize_rwlock(rwlock* rwlock_p);

// majorly the api only has below 6 functions

#define READ_PREFERING 0
#define WRITE_PREFERING 1

void read_lock(rwlock* rwlock_p, int non_blocking, int prefering);
void write_lock(rwlock* rwlock_p, int non_blocking);

int downgrade_lock(rwlock* rwlock_p);
int upgrade_lock(rwlock* rwlock_p, int non_blocking);

int read_unlock(rwlock* rwlock_p);
int write_unlock(rwlock* rwlock_p);

// use the below 4 functions only with an external lock held

int is_read_locked(rwlock* rwlock_p);
int is_write_locked(rwlock* rwlock_p);
int has_waiters(rwlock* rwlock_p);
int is_referenced(rwlock* rwlock_p);

#endif