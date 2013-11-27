//Sony Computer Entertainment Confidential
#include "pthread/include/pthread.h"

/* Common definitions */
#define VALID(rwlock) \
	(((rwlock) != 0) && ((rwlock)->rdwrQ != INVALID_ID_))
#define INVALIDATE(rwlock) \
	do { (rwlock)->rdwrQ = INVALID_ID_; } while(0)
#define STATIC_INIT(rwlock) \
	(((rwlock)->rdwrQ == STATIC_INIT_ID_) && ((rwlock)->writelock == RWLOCK_SIG_))

#define MAX_COUNT 0x7fffffff    

/*
 * The pthread_rwlock_init subroutine initializes the read-write
 * lock referenced by rwlock with the attributes referenced by 
 * attr. If attr is NULL, the default read-write lock attributes 
 * are used; the effect is the same as passing the address of a 
 * default read-write lock attributes object. Once initialized, 
 * the lock can be used any number of times without being 
 * re-initialized. Upon successful initialization, the state of 
 * the read-write lock becomes initialized and unlocked. Results 
 * are undefined if pthread_rwlock_init is called specifying an 
 * already initialized read-write lock. Results are undefined if 
 * a read-write lock is used without first being initialized.
 *
 * If the pthread_rwlock_init function fails, rwlock is not 
 * initialized and the contents of rwlock are undefined.
 *
 * The pthread_rwlock_destroy function destroys the read-write 
 * lock object referenced by rwlock and releases any resources 
 * used by the lock. The effect of subsequent use of the lock 
 * is undefined until the lock is re-initialized by another call 
 * to pthread_rwlock_init. An implementation may cause 
 * pthread_rwlock_destroy to set the object referenced by rwlock 
 * to an invalid value. Results are undefined if 
 * pthread_rwlock_destroy is called when any thread holds rwlock. 
 * Attempting to destroy an uninitialized read-write lock results 
 * in undefined behavior. A destroyed read-write lock object can 
 * be re-initialized using pthread_rwlock_init; the results of 
 * otherwise referencing the read-write lock object after it has 
 * been destroyed are undefined.
 *
 * In cases where default read-write lock attributes are
 * appropriate, the macro PTHREAD_RWLOCK_INITIALIZER can be used 
 * to initialize read-write locks that are statically allocated. 
 * The effect is equivalent to dynamic initialization by a call
 * to pthread_rwlock_init with the parameter attr specified as 
 * NULL, except that no error checks are performed.
 */

EXTERN int pthread_rwlock_destroy(pthread_rwlock_t *rwlock)
{
  int res;
  SceUInt32 nbThreads;

  if (!VALID(rwlock)) return EINVAL;

  ENTER_CRITICAL();

  if (!STATIC_INIT(rwlock))
  {
    res = sceKernelCancelSema(rwlock->rdwrQ, -1, &nbThreads);
    sceCHECK(res);

    res = sceKernelDeleteSema(rwlock->rdwrQ);
    sceCHECK(res);
  }

  INVALIDATE(rwlock);
  LEAVE_CRITICAL();
  return 0;
}


EXTERN int pthread_rwlock_init(pthread_rwlock_t *rwlock,
                               const pthread_rwlockattr_t *rwlockattr)
{
  int res;

  // Unused parameters
  (void)&rwlockattr;

  CHECK_PT_PTR(rwlock);

  ENTER_CRITICAL();

  res = sceKernelCreateSema("pthread rwlock",
                            SCE_KERNEL_ATTR_TH_FIFO,
                            MAX_COUNT, MAX_COUNT,
                            NULL);
  if (res > 0) 
    {
      rwlock->rdwrQ = res;
      rwlock->writelock = 0;
      res = 0;
    }
  else
    {
      sceCHECK(res);
      if (!STATIC_INIT(rwlock))
        INVALIDATE(rwlock);
    }

  LEAVE_CRITICAL();
  return res;
}


static int init_static(pthread_rwlock_t *rwlock)
{
  PTHREAD_INIT();
  int res = 0;

  ENTER_CRITICAL();
  if (STATIC_INIT(rwlock))
    res = pthread_rwlock_init(rwlock, NULL);
  LEAVE_CRITICAL();
  return res;
}


/* 
 * The pthread_rwlock_rdlock function applies a read lock to
 * the read-write lock referenced by rwlock. The calling thread
 * acquires the read lock if a writer does not hold the lock
 * and there are no writers blocked on the lock. It is unspecified 
 * whether the calling thread acquires the lock when a writer does
 * not hold the lock and there are writers waiting for the lock. 
 * If a writer holds the lock, the calling thread will not acquire
 * the read lock. If the read lock is not acquired, the calling
 * thread blocks (that is, it does not return from the
 * pthread_rwlock_rdlock call) until it can acquire the lock.
 * Results are undefined if the calling thread holds a write lock
 * on rwlock at the time the call is made.
 *
 * Implementations are allowed to favor writers over readers to
 * avoid writer starvation.
 *
 * A thread may hold multiple concurrent read locks on rwlock
 * (that is, successfully call the pthread_rwlock_rdlock
 * function n times). If so, the thread must perform matching 
 * unlocks (that is, it must call the pthread_rwlock_unlock
 * function n times).
 *
 * The function pthread_rwlock_tryrdlock applies a read lock as 
 * in the pthread_rwlock_rdlock function with the exception
 * that the function fails if any thread holds a write lock on
 * rwlock or there are writers blocked on rwlock.
 */

static int lock(pthread_rwlock_t *rwlock, SceUInt *t, int write)
{
  int res;

  if (!VALID(rwlock)) return EINVAL;
  
  if (STATIC_INIT(rwlock))
    {
	  res = init_static(rwlock);
	  if (res) return res;
    }

  res = sceKernelWaitSema(rwlock->rdwrQ, write ? MAX_COUNT : 1, t);

  if (res == SCE_OK) 
    {
      rwlock->writelock = write;
      return 0;
    }
  else if (res == (int)SCE_KERNEL_ERROR_WAIT_TIMEOUT)
    return ETIMEDOUT;
  else
    return EINVAL;
}


EXTERN int pthread_rwlock_rdlock(pthread_rwlock_t *rwlock)
{
  return lock(rwlock, NULL, 0);
}


EXTERN int pthread_rwlock_timedrdlock(pthread_rwlock_t *rwlock, 
                                      const struct timespec *abstime)
{
  SceUInt delta;

  if (abstime == NULL)
    return EINVAL;
  
  delta = getDeltaTime(abstime);

  return lock(rwlock, &delta, 0);
}


EXTERN int pthread_rwlock_tryrdlock(pthread_rwlock_t *rwlock)
{
  int res;

  if (!VALID(rwlock)) return EINVAL;

  if (STATIC_INIT(rwlock))
    {
      res = init_static(rwlock);
	  if (res) return res;
    }

  res = sceKernelPollSema(rwlock->rdwrQ, 1);
  if (res == SCE_OK)
    return 0;
  if (res == (int)SCE_KERNEL_ERROR_SEMA_ZERO)
    return EBUSY;
  return EINVAL;
}


/*
 * The pthread_rwlock_wrlock subroutine applies a write 
 * lock to the read-write lock referenced by rwlock. The 
 * calling thread acquires the write lock if no other
 * thread (reader or writer) holds the read-write lock rwlock. 
 * Otherwise, the thread blocks (that is, does not return 
 * from the pthread_rwlock_wrlock call) until it can acquire
 * the lock. Results are undefined if the calling thread
 * holds the read-write lock (whether a read or write lock)
 * at the time the call is made.
 *
 * Implementations are allowed to favor writers over
 * readers to avoid writer starvation.
 *
 * The pthread_rwlock_trywrlock subroutine applies a write 
 * lock like the pthread_rwlock_wrlock subroutine, with the
 * exception that the function fails if any thread currently 
 * holds rwlock (for reading or writing).
 */

EXTERN int pthread_rwlock_wrlock(pthread_rwlock_t *rwlock)
{
  return lock(rwlock, NULL, 1);
}


EXTERN int pthread_rwlock_timedwrlock(pthread_rwlock_t *rwlock,
                                      const struct timespec *abstime)
{
  SceUInt delta;

  if (abstime == NULL)
    return EINVAL;
  
  delta = getDeltaTime(abstime);

  return lock(rwlock, &delta, 1);
}


EXTERN int pthread_rwlock_trywrlock(pthread_rwlock_t *rwlock)
{
  int res, ret = EINVAL;

  if (!VALID(rwlock)) return EINVAL;

  if (STATIC_INIT(rwlock))
    {
      res = init_static(rwlock);
      if (res) return res;
    }

  // Acquire the semaphore with ALL the resources that
  // way no other readers or writers will be granted the 
  // semaphore
  res = sceKernelPollSema(rwlock->rdwrQ, MAX_COUNT);
  if (res == SCE_OK)
    {
      rwlock->writelock = 1;
      ret = 0;
    }
  else if (res == (int)SCE_KERNEL_ERROR_SEMA_ZERO)
    ret = EBUSY;
  else
    sceCHECK(ret);

  return ret;
}


/*
 * The pthread_rwlock_unlock subroutine is called to release 
 * a lock held on the read-write lock object referenced by
 * rwlock. Results are undefined if the read-write lock rwlock 
 * is not held by the calling thread.
 *
 * If this subroutine is called to release a read lock from 
 * the read-write lock object and there are other read locks 
 * currently held on this read-write lock object, the read-write 
 * lock object remains in the read locked state. If this 
 * subroutine releases the calling thread's last read lock on this 
 * read-write lock object, then the calling thread is no longer 
 * one of the owners of the object. If this subroutine releases 
 * the last read lock for this read-write lock object, the 
 * read-write lock object will be put in the unlocked state with 
 * no owners.
 *
 * If this subroutine is called to release a write lock for this 
 * read-write lock object, the read-write lock object will be put
 * in the unlocked state with no owners.
 *
 * If the call to the pthread_rwlock_unlock subroutine results
 * in the read-write lock object becoming unlocked and there are
 * multiple threads waiting to acquire the read-write lock object 
 * for writing, the scheduling policy is used to determine which
 * thread acquires the read-write lock object for writing. If 
 * there are multiple threads waiting to acquire the read-write
 * lock object for reading, the scheduling policy is used to 
 * determine the order in which the waiting threads acquire the 
 * read-write lock object for reading. If there are multiple
 * threads blocked on rwlock for both read locks and write locks, 
 * it is unspecified whether the readers acquire the lock first 
 * or whether a writer acquires the lock first.
 */

EXTERN int pthread_rwlock_unlock(pthread_rwlock_t *rwlock)
{
  int count, res;

  if (!VALID(rwlock)) return EINVAL;

  if (rwlock->writelock)
    {
      rwlock->writelock = 0;
      count = MAX_COUNT;
    }
  else
    count = 1;

  res = sceKernelSignalSema(rwlock->rdwrQ, count);

  if (res == SCE_OK)
    return 0;
  else
    return EINVAL;
}


EXTERN int pthread_rwlockattr_destroy(pthread_rwlockattr_t *rwlockattr)
{
  // Unused parameters
  (void)&rwlockattr;
  return 0;
}


EXTERN int pthread_rwlockattr_init(pthread_rwlockattr_t *rwlockattr)
{
  // Unused parameters
  (void)&rwlockattr;
  return 0;
}
