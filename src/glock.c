#include<lockking/glock.h>

#include<stdlib.h>
#include<string.h>

#include<posixutils/pthread_cond_utils.h>

// for internal use only
static inline int are_glock_modes_compatible_UNSAFE(const glock_matrix* gmatr, uint64_t M1, uint64_t M2)
{
	return !!(gmatr->matrix[GLOCK_MATRIX_INDEX(M1, M2)]);
}

int are_glock_modes_compatible(const glock_matrix* gmatr, uint64_t M1, uint64_t M2)
{
	// M1 is out of bounds
	if(M1 >= gmatr->lock_modes_count)
		return 0;

	// M2 is out of bounds
	if(M2 >= gmatr->lock_modes_count)
		return 0;

	return are_glock_modes_compatible_UNSAFE(gmatr, M1, M2);
}

static inline pthread_mutex_t* get_glock_lock(glock* glock_p)
{
	if(glock_p->has_internal_lock)
		return &(glock_p->internal_lock);
	else
		return glock_p->external_lock;
}

int initialize_glock(glock* glock_p, const glock_matrix* gmatr, pthread_mutex_t* external_lock)
{
	glock_p->locks_granted_count_per_lock_mode = malloc(sizeof(uint64_t) * gmatr->lock_modes_count);
	if(glock_p->locks_granted_count_per_lock_mode == NULL)
		return 0;
	memset(glock_p->locks_granted_count_per_lock_mode, 0, sizeof(uint64_t) * gmatr->lock_modes_count);

	glock_p->gmatr = gmatr;
	if(external_lock)
	{
		glock_p->has_internal_lock = 0;
		glock_p->external_lock = external_lock;
	}
	else
	{
		glock_p->has_internal_lock = 1;
		pthread_mutex_init(&(glock_p->internal_lock), NULL);
	}
	glock_p->waiters_count = 0;
	pthread_cond_init_with_monotonic_clock(&(glock_p->wait));
	return 1;
}

void deinitialize_glock(glock* glock_p)
{
	free(glock_p->locks_granted_count_per_lock_mode);
	if(glock_p->has_internal_lock)
		pthread_mutex_destroy(&(glock_p->internal_lock));
	pthread_cond_destroy(&(glock_p->wait));
}

// lock_mde must be within bounds
static inline int can_grab_lock(const glock* glock_p, uint64_t lock_mode)
{
	for(uint64_t i = 0; i < glock_p->gmatr->lock_modes_count; i++)
	{
		// you can not grab lock,
		// if your lock_mode is incompatible with any of the lock_modes that have been already granted
		if((glock_p->locks_granted_count_per_lock_mode[i] > 0) && !are_glock_modes_compatible_UNSAFE(glock_p->gmatr, i, lock_mode))
			return 0;
	}
	return 1;
}

int glock_lock(glock* glock_p, uint64_t lock_mode, uint64_t timeout_in_microseconds)
{
	// lock_mode must be within bounds
	if(lock_mode >= glock_p->gmatr->lock_modes_count)
		return 0;

	int res = 0;

	if(glock_p->has_internal_lock)
		pthread_mutex_lock(get_glock_lock(glock_p));

	if(timeout_in_microseconds != NON_BLOCKING) // you are allowed to block only if (timeout_in_microseconds != NON_BLOCKING)
	{
		int wait_error = 0;
		while(!can_grab_lock(glock_p, lock_mode) && !wait_error) // block while you can not grab lock and there is no wait error
		{
			glock_p->waiters_count++;
			if(timeout_in_microseconds == BLOCKING)
				wait_error = pthread_cond_wait(&(glock_p->wait), get_glock_lock(glock_p));
			else
				wait_error = pthread_cond_timedwait_for_microseconds(&(glock_p->wait), get_glock_lock(glock_p), &timeout_in_microseconds);
			glock_p->waiters_count--;
		}
	}

	// if you can grab a lock, then grab it, else fail
	if(can_grab_lock(glock_p, lock_mode))
	{
		glock_p->locks_granted_count_per_lock_mode[lock_mode]++;
		res = 1;
	}

	if(glock_p->has_internal_lock)
		pthread_mutex_unlock(get_glock_lock(glock_p));

	return res;
}

// do ensure that you have the old_lock_mode held
static inline int can_transition_lock(glock* glock_p, uint64_t old_lock_mode, uint64_t new_lock_mode)
{
	// assume you do not have the lock you currently hold
	glock_p->locks_granted_count_per_lock_mode[old_lock_mode]--;

	// now check if the other lock holders are okay with you having the lock in the new_lock_mode
	int can_transition = can_grab_lock(glock_p, new_lock_mode);

	// rever the prior change and return the result
	glock_p->locks_granted_count_per_lock_mode[old_lock_mode]++;
	return can_transition;
}

int glock_transition_lock(glock* glock_p, uint64_t old_lock_mode, uint64_t new_lock_mode, uint64_t timeout_in_microseconds)
{
	// lock_mode must be within bounds
	if(old_lock_mode >= glock_p->gmatr->lock_modes_count)
		return 0;

	// lock_mode must be within bounds
	if(new_lock_mode >= glock_p->gmatr->lock_modes_count)
		return 0;

	int res = 0;

	if(glock_p->has_internal_lock)
		pthread_mutex_lock(get_glock_lock(glock_p));

	// make sure that the resource is locked
	if(glock_p->locks_granted_count_per_lock_mode[old_lock_mode] == 0)
		goto EXIT;

	if(timeout_in_microseconds != NON_BLOCKING) // you are allowed to block only if (timeout_in_microseconds != NON_BLOCKING)
	{
		int wait_error = 0;
		while(!can_transition_lock(glock_p, old_lock_mode, new_lock_mode) && !wait_error) // block while you can not transition lock and there is no wait error
		{
			glock_p->waiters_count++;
			if(timeout_in_microseconds == BLOCKING)
				wait_error = pthread_cond_wait(&(glock_p->wait), get_glock_lock(glock_p));
			else
				wait_error = pthread_cond_timedwait_for_microseconds(&(glock_p->wait), get_glock_lock(glock_p), &timeout_in_microseconds);
			glock_p->waiters_count--;
		}
	}

	if(can_transition_lock(glock_p, old_lock_mode, new_lock_mode))
	{
		// transition the lock
		glock_p->locks_granted_count_per_lock_mode[old_lock_mode]--;
		glock_p->locks_granted_count_per_lock_mode[new_lock_mode]++;
		res = 1;

		// wake up any waiters, we changed lock_mode, there could be some lock_mode waiter that could have become compatible with other threads
		if(glock_p->waiters_count > 0)
			pthread_cond_broadcast(&(glock_p->wait));
	}

	EXIT:;
	if(glock_p->has_internal_lock)
		pthread_mutex_unlock(get_glock_lock(glock_p));

	return res;
}

int glock_unlock(glock* glock_p, uint64_t lock_mode)
{
	// lock_mode must be within bounds
	if(lock_mode >= glock_p->gmatr->lock_modes_count)
		return 0;

	int res = 0;

	if(glock_p->has_internal_lock)
		pthread_mutex_lock(get_glock_lock(glock_p));

	// make sure that the resource is locked
	if(glock_p->locks_granted_count_per_lock_mode[lock_mode] == 0)
		goto EXIT;

	// decrement the locks_granted_count, releasing lock for the specific lock_mode
	glock_p->locks_granted_count_per_lock_mode[lock_mode]--;
	res = 1;

	// wake up any waiters
	if(glock_p->waiters_count > 0)
		pthread_cond_broadcast(&(glock_p->wait));

	EXIT:;
	if(glock_p->has_internal_lock)
		pthread_mutex_unlock(get_glock_lock(glock_p));

	return res;
}