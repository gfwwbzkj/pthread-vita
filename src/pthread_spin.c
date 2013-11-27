//Sony Computer Entertainment Confidential
#include "pthread/include/pthread.h"

/* Common definitions */
#define VALID(lock) \
	((lock != 0) && (lock->lock != 2))
#define INVALIDATE(lock) \
	(lock->lock = 2)


/*
 * The pthread_spin_destroy subroutine destroys the spin 
 * lock referenced by lock and releases any resources used 
 * by the lock. The effect of subsequent use of the lock is 
 * undefined until the lock is reinitialized by another call 
 * to the pthread_spin_init subroutine. The results are 
 * undefined if the pthread_spin_destroy subroutine is called 
 * when a thread holds the lock, or if this function is 
 * called with an uninitialized thread spin lock.
 */

EXTERN int pthread_spin_destroy(pthread_spinlock_t *lock)
{
  if (!VALID(lock)) return EINVAL;
  INVALIDATE(lock);
  return 0;
}


EXTERN int pthread_spin_init(pthread_spinlock_t *lock, int pshared)
{
  CHECK_PT_PTR(lock);
  // Unused parameters
  (void)&pshared;
  lock->lock = 0;
  return 0;
}


/*
 * The pthread_spin_lock subroutine locks the spin lock 
 * referenced by the lock parameter. The calling thread 
 * shall acquire the lock if it is not held by another 
 * thread. Otherwise, the thread spins (that is, does not 
 * return from the pthread_spin_lock call) until the lock 
 * becomes available. The results are undefined if the calling 
 * thread holds the lock at the time the call is made. The 
 * pthread_spin_trylock subroutine locks the spin lock referenced
 * by the lock parameter if it is not held by any thread. 
 * Otherwise, the function fails.
 * 
 * The results are undefined if any of these subroutines is 
 * called with an uninitialized spin lock.
 */

EXTERN int pthread_spin_lock(pthread_spinlock_t *lock)
{
  if (!VALID(lock)) return EINVAL;
  while (test_and_set_bit(0, &lock->lock) != 0)
    ;
  return 0;
}


EXTERN int pthread_spin_trylock(pthread_spinlock_t *lock)
{
  if (!VALID(lock)) return EINVAL;
  if (test_and_set_bit(0, &lock->lock) == 0)
    return 0;
  else
    return EBUSY;
}


/*
 * The pthread_spin_unlock subroutine releases the spin lock 
 * referenced by the lock parameter which was locked using 
 * the pthread_spin_lock subroutine or the pthread_spin_trylock 
 * subroutine. The results are undefined if the lock is not held
 * by the calling thread. If there are threads spinning on the 
 * lock when the pthread_spin_unlock subroutine is called, the 
 * lock becomes available and an unspecified spinning thread 
 * shall acquire the lock.
 *
 * The results are undefined if this subroutine is called with
 * an uninitialized thread spin lock.
 */

EXTERN int pthread_spin_unlock(pthread_spinlock_t *lock)
{
  if (!VALID(lock)) return EINVAL;
  if (test_and_clear_bit(0, &lock->lock))
    return 0;
  else
    return EPERM;
}
