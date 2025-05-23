#ifndef GLOCK_H
#define GLOCK_H

#include<pthread.h>
#include<stdint.h>

#include<cutlery/cutlery_math.h> // using min() and max() macros

#include<posixutils/timespec_utils.h>

/*
	glock is short for a Generalized Lock
	It works on the principles if a lock-compatibility-matrix
	This lock has a variety of lock modes, unlike a rwlock it  is not just a Reader and Writer mode lock
	This matrix the lock-compatibility-matrix controls which lock-modes could be held concurrently
	From here on the lock-compatibility-matrix would be called glock_matrix and the lock will be called a glock
*/

typedef struct glock_matrix glock_matrix;
struct glock_matrix
{
	uint64_t lock_modes_count; // this is the number of lock modes that this matrix supports
	// for instance a mutex has only 1 lock mode
	// while a rwlock has 2 lock modes 1 for read mode and another for write mode

	uint8_t* matrix; // this matirx is not just a linear array
	// the matrix ideally should consists of only 0 or 1 values, on all of the cells, and must be a symmetric matrix
	// if matrix[M1][M2] says that the modes are compatible, the the same should be said for the matrix[M2][M1]
	// for this reason the matrix is arranges as follows
	/*
		for 4 lock modes, the matrix looks like this
		matrix = {
		//  M0    M1    M2    M3
			0,                    // M0
			1,    2,              // M1
			3,    4,    5,        // M2
			6,    7,    8,    9,  // M3
		}
		see how the upper part of the matrix is left empty to avoid duplication
		if the lock modes are depicted by M0, M1, M2, M3
	*/
};

#define MAKE_UINT64(x) ((uint64_t)(x))

// gives you the first index of the x-th (0 indexed) row
#define GLOCK_MATRIX_BASE(x) ((MAKE_UINT64(x)*(MAKE_UINT64(x)+UINT64_C(1)))/UINT64_C(2))

// gives the number of elements indexable for glock_matrix.matrix, when there are lock_modes_count number of lock_modes
// same as lock_modes * (lock_modes + 1) / 2
#define GLOCK_MATRIX_SIZE(lock_modes_count) GLOCK_MATRIX_BASE(lock_modes_count)

// gives the index for the corresponding pair of lock modes in the glock_matrix.matrix
// same as min(M1,M2) + ( ( max(M1,M2) * (max(M1,M2)+1) ) /2 )
#define GLOCK_MATRIX_INDEX(M1, M2) (min(MAKE_UINT64(M1),MAKE_UINT64(M2))+GLOCK_MATRIX_BASE(max(MAKE_UINT64(M1),MAKE_UINT64(M2))))

int are_glock_modes_compatible(const glock_matrix* gmatr, uint64_t M1, uint64_t M2);

/*
	For a traditional database hierarchial locks the glock_matrix definition looks like below

#define MODES_COUNT 4

#define ISmode 0
#define IXmode 1
#define Smode  2
#define Xmode  3

const glock_matrix hmat = {
	.lock_modes_count = MODES_COUNT,
	.matrix = (uint8_t[GLOCK_MATRIX_SIZE(MODES_COUNT)]){
		// IS  IX  S   X
		   1,               // IS
		   1,  1,           // IX
		   1,  0,  1,       // S
		   0,  0,  0,  0,   // X
	},
};
*/

//-----------------------------------------------------------------------------
//------------- GLOCK - the lock itself is implemented below ------------------
//-----------------------------------------------------------------------------

typedef struct glock glock;
struct glock
{
	int has_internal_lock : 1;
	union{
		pthread_mutex_t internal_lock;
		pthread_mutex_t* external_lock;
	};

	pthread_cond_t wait;
	uint64_t waiters_count; // number of waiters waiting on the wait condition variable

	uint64_t* locks_granted_count_per_lock_mode; // array of size lock_modes_count, 1 counter for each lock mode, this is dynamically allocated array

	const glock_matrix* gmatr;
};

int initialize_glock(glock* glock_p, const glock_matrix* gmatr, pthread_mutex_t* external_lock);
void deinitialize_glock(glock* glock_p);

int glock_lock(glock* glock_p, uint64_t lock_mode, uint64_t timeout_in_microseconds);
int glock_transition_lock(glock* glock_p, uint64_t old_lock_mode, uint64_t new_lock_mode, uint64_t timeout_in_microseconds);
int glock_unlock(glock* glock_p, uint64_t lock_mode);

// use the below 3 functions only with an external_lock held, else they give only instantaneous results

int is_glock_locked(glock* glock_p);
int has_glock_waiters(glock* glock_p);
int is_glock_referenced(glock* glock_p);

#endif