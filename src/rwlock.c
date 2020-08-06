#include<rwlock.h>

rwlock* get_rwlock()
{
	rwlock* rwlock_p = malloc(sizeof(rwlock));
	initialize_rwlock(rwlock_p);
	return rwlock_p;
}

void initialize_rwlock(rwlock* rwlock_p)
{
	rwlock_p->reader_threads_waiting = 0;
	rwlock_p->reading_threads = 0;
	rwlock_p->writer_threads_waiting = 0;
	rwlock_p->writing_threads = 0;
	pthread_mutex_init(&(rwlock_p->internal_protector), NULL);
	pthread_cond_init(&(rwlock_p->read_wait), NULL);
	pthread_cond_init(&(rwlock_p->write_wait), NULL);
}

void read_lock(rwlock* rwlock_p)
{
	pthread_mutex_lock(&(rwlock_p->internal_protector));

	// if writers already have the lock, or they want the lock, we go for a wait,
	// so basically if any writer is writing or wants to write, we go to wait
	while(rwlock_p->writing_threads > 0 || (rwlock_p->reading_threads == 0 && rwlock_p->writer_threads_waiting > 0))
	{
		rwlock_p->reader_threads_waiting++;
		pthread_cond_wait(&(rwlock_p->read_wait), &(rwlock_p->internal_protector));
		rwlock_p->reader_threads_waiting--;
	}

	// increment the number of reading threads
	rwlock_p->reading_threads++;

	pthread_mutex_unlock(&(rwlock_p->internal_protector));
}

/*
		Policy defined when shall a reader take the lock
		Mandatory condition, There must be no thread writing

							writer_threads_waiting 		==0						>0
		reading_threads
				==0									take read lock 				wait
				>0 									take read lock 			take read lock
*/

void read_unlock(rwlock* rwlock_p)
{
	pthread_mutex_lock(&(rwlock_p->internal_protector));

	// decrement the number of reading threads
	rwlock_p->reading_threads--;

	// if this is the last read thread, and hence it is mandatory that reader_threads_waiting = 0
	// because otherwise they would have already entered,
	// so if there are any waiting writer threads, we would just, wake one of them up
	if(rwlock_p->reading_threads == 0 && rwlock_p->writer_threads_waiting > 0)
	{
		pthread_cond_signal(&(rwlock_p->write_wait));
	}

	pthread_mutex_unlock(&(rwlock_p->internal_protector));
}

void write_lock(rwlock* rwlock_p)
{
	pthread_mutex_lock(&(rwlock_p->internal_protector));

	// writers need exclusive lock, the total number of threads reading or writing, must be 0, for a writer thread to acquire lock 
	while(rwlock_p->reading_threads + rwlock_p->writing_threads > 0)
	{
		rwlock_p->writer_threads_waiting++;
		pthread_cond_wait(&(rwlock_p->write_wait), &(rwlock_p->internal_protector));
		rwlock_p->writer_threads_waiting--;
	}

	// increase the number, of writing threads
	rwlock_p->writing_threads++;

	pthread_mutex_unlock(&(rwlock_p->internal_protector));
}

void write_unlock(rwlock* rwlock_p)
{
	pthread_mutex_lock(&(rwlock_p->internal_protector));

	// decrement the number of writing threads
	rwlock_p->writing_threads--;

	// if there are asny writing threads, we wake them up
	if(rwlock_p->writer_threads_waiting > 0)
	{
		pthread_cond_signal(&(rwlock_p->write_wait));
	}
	// else we wake up all the reading threads
	else if(rwlock_p->reader_threads_waiting > 0)
	{
		pthread_cond_broadcast(&(rwlock_p->read_wait));
	}

	pthread_mutex_unlock(&(rwlock_p->internal_protector));
}

void downgrade_writer_to_reader_lock(rwlock* rwlock_p)
{
	pthread_mutex_lock(&(rwlock_p->internal_protector));

	// you must be a writer to call this function
	rwlock_p->writing_threads--;

	// count your self in as a reader
	rwlock_p->reading_threads++;

	// else we wake up all the reading threads
	if(rwlock_p->reader_threads_waiting > 0)
	{
		pthread_cond_broadcast(&(rwlock_p->read_wait));
	}

	pthread_mutex_unlock(&(rwlock_p->internal_protector));
}

unsigned int get_readers_count(rwlock* rwlock_p)
{
	pthread_mutex_lock(&(rwlock_p->internal_protector));

	unsigned int count = rwlock_p->reading_threads;

	pthread_mutex_unlock(&(rwlock_p->internal_protector));

	return count;
}

unsigned int get_writers_count(rwlock* rwlock_p)
{
	pthread_mutex_lock(&(rwlock_p->internal_protector));

	unsigned int count = rwlock_p->writing_threads;

	pthread_mutex_unlock(&(rwlock_p->internal_protector));

	return count;
}

unsigned int get_waiting_readers_count(rwlock* rwlock_p)
{
	pthread_mutex_lock(&(rwlock_p->internal_protector));

	unsigned int count = rwlock_p->reader_threads_waiting;

	pthread_mutex_unlock(&(rwlock_p->internal_protector));

	return count;
}

unsigned int get_waiting_writers_count(rwlock* rwlock_p)
{
	pthread_mutex_lock(&(rwlock_p->internal_protector));

	unsigned int count = rwlock_p->writer_threads_waiting;

	pthread_mutex_unlock(&(rwlock_p->internal_protector));

	return count;
}

unsigned int get_total_thread_count(rwlock* rwlock_p)
{
	pthread_mutex_lock(&(rwlock_p->internal_protector));

	unsigned int count = rwlock_p->reading_threads + rwlock_p->writing_threads 
	 + rwlock_p->reader_threads_waiting + rwlock_p->writer_threads_waiting;

	pthread_mutex_unlock(&(rwlock_p->internal_protector));

	return count;
}

int deinitialize_rwlock(rwlock* rwlock_p)
{
	// we can not exit, if any thread is operating on the data
	if(rwlock_p->reading_threads + rwlock_p->writing_threads + 
		rwlock_p->reader_threads_waiting + rwlock_p->writer_threads_waiting > 0)
	{
		return -1;
	}
	pthread_mutex_destroy(&(rwlock_p->internal_protector));
	pthread_cond_destroy(&(rwlock_p->read_wait));
	pthread_cond_destroy(&(rwlock_p->write_wait));
	return 0;
}

int delete_rwlock(rwlock* rwlock_p)
{
	if(deinitialize_rwlock(rwlock_p) == 0)
	{
		free(rwlock_p);
		return 0;
	}
	return -1;
}