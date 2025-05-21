#include<lockking/glock.h>

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