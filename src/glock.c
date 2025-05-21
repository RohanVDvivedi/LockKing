#include<lockking/glock.h>

#include<stdlib.h>
#include<string.h>

#include<posixutils/pthread_cond_utils.h>

// for internal use only
static int are_glock_modes_compatible_UNSAFE(const glock_matrix* gmatr, uint64_t M1, uint64_t M2)
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

int initialize_glock(glock* glock_p, const glock_matrix* gmatr, pthread_mutex_t* external_lock)
{
	glock_p->locks_granted_count_per_glock_mode = malloc(sizeof(uint64_t) * gmatr->lock_modes_count);
	if(glock_p->locks_granted_count_per_glock_mode == NULL)
		return 0;
	memset(glock_p->locks_granted_count_per_glock_mode, 0, sizeof(uint64_t) * gmatr->lock_modes_count);

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
	free(glock_p->locks_granted_count_per_glock_mode);
	if(glock_p->has_internal_lock)
		pthread_mutex_destroy(&(glock_p->internal_lock));
	pthread_cond_destroy(&(glock_p->wait));
}