#include<rwlock.h>

void initialize_rwlock(rwlock* rwlock_p, pthread_mutex_t* external_lock);
int deinitialize_rwlock(rwlock* rwlock_p);

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