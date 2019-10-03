#ifndef RW_LOCK_H
#define RW_LOCK_H

#include<stdio.h>
#include<stdlib.h>

#include<pthread.h>

typedef struct rwlock rwlock;
struct rwlock
{
	unsigned int reader_threads_waiting;	// [ 0 .. INT_MAX ] 
	unsigned int reading_threads;			// [ 0 .. INT_MAX ] 

	unsigned int writer_threads_waiting;	// [ 0 .. INT_MAX ] 
	unsigned int writing_threads;			// [ 0 .. 1 ] 

	pthread_mutex_t internal_protector;

	pthread_cond_t read_wait;
	pthread_cond_t write_wait;
};

rwlock* get_rwlock();

void read_lock(rwlock* rwlock_p);

void read_unlock(rwlock* rwlock_p);

void write_lock(rwlock* rwlock_p);

void write_unlock(rwlock* rwlock_p);

// if some one is already holding a read or write lock, we can not delete the lock
// if we could not delete the lock, it function returns -1, else 0
int delete_rwlock(rwlock* rwlock_p);

#endif