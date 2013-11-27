//Sony Computer Entertainment Confidential
#include "pthread/include/pthread.h"
#include <string.h> // memset

/* Common definitions */
#define VALID(mutex) \
	(((mutex) != 0) && ((mutex)->id != INVALID_ID_))
#define INVALIDATE(mutex) \
	do { (mutex)->id = INVALID_ID_; } while(0)
#define STATIC_INIT(mutex) \
	(((mutex)->id == STATIC_INIT_ID_) && ((mutex)->owner == (pthread_t)MUTEX_SIG_))

/*
 * The pthread_mutex_init function initializes the mutex referenced
 * by mutex with attributes specified by attr. If attr is NULL, the 
 * default mutex attributes are used; the effect is the same as passing 
 * the address of a default mutex attributes object. Upon successful 
 * initialization, the state of the mutex becomes initialized and 
 * unlocked.
 *
 * Attempting to initialize an already initialized mutex results 
 * in undefined behavior.
 *
 * The pthread_mutex_destroy function destroys the mutex object 
 * referenced by mutex; the mutex object becomes, in effect, 
 * uninitialized. An implementation may cause pthread_mutex_destroy 
 * to set the object referenced by mutex to an invalid value. A 
 * destroyed mutex object can be re-initialized using 
 * pthread_mutex_init; the results of otherwise referencing the 
 * object after it has been destroyed are undefined.
 *
 * It is safe to destroy an initialized mutex that is unlocked. 
 * Attempting to destroy a locked mutex results in undefined behavior.
 *
 * In cases where default mutex attributes are appropriate, the macro 
 * PTHREAD_MUTEX_INITIALIZER can be used to initialize mutexes that 
 * are statically allocated. The effect is equivalent to dynamic 
 * initialization by a call to pthread_mutex_init with parameter attr 
 * specified as NULL, except that no error checks are performed.
 */

EXTERN int pthread_mutex_destroy(pthread_mutex_t *mutex)
{
  int res;
  SceUInt32 nbThreads;

  if (!VALID(mutex)) return EINVAL;

  ENTER_CRITICAL();

  res = sceKernelCancelSema(mutex->id, -1, &nbThreads);
  sceCHECK(res);

  res = sceKernelDeleteSema(mutex->id);
  sceCHECK(res);

  INVALIDATE(mutex);

  LEAVE_CRITICAL();
  return 0;
}


EXTERN int pthread_mutex_init(pthread_mutex_t *restrict mutex,
                              const pthread_mutexattr_t *restrict attr)
{
  pthread_mutexattr_t a;
  int res;

  CHECK_PT_PTR(mutex);

  ENTER_CRITICAL();

  if (attr == NULL) 
    pthread_mutexattr_init(&a);
  else
    a = *attr;

  res = sceKernelCreateSema("pthread mutex",
                            a.scheduling,
                            a.MaxCount, a.MaxCount,
                            NULL);
  if (res > 0) 
    {
      mutex->id = res;
      mutex->type = a.type;
      mutex->recursivecount = 0;
      mutex->owner = NULL;
      mutex->protocol = a.protocol;
      mutex->prioceiling = a.prioceiling;
      memset(mutex->priowait, 0, sizeof(mutex->priowait));

      res = 0;
      goto exit;
    }
  else
    {
		sceCHECK(res);
		if (!STATIC_INIT(mutex))
			INVALIDATE(mutex);
    }

  res = ERROR_errno_sce(res);

 exit:
  LEAVE_CRITICAL();
  return res;
}


static int init_static(pthread_mutex_t *mutex)
{
  int rc = 0, type;
  pthread_mutexattr_t attr;

  ENTER_CRITICAL();

  if (STATIC_INIT(mutex))
    {
      type = mutex->type;

      pthread_mutexattr_init(&attr);
      pthread_mutexattr_settype(&attr, type);
      rc = pthread_mutex_init(mutex, &attr);
    }

  LEAVE_CRITICAL();
  return rc;
}


/*
 * The mutex object referenced by the mutex parameter is locked 
 * by calling pthread_mutex_lock. If the mutex is already locked, 
 * the calling thread blocks until the mutex becomes available. This 
 * operation returns with the mutex object referenced by the mutex 
 * parameter in the locked state with the calling thread as its owner.
 *
 * If the mutex type is PTHREAD_MUTEX_NORMAL, deadlock detection is
 * not provided. Attempting to relock the mutex causes deadlock. If 
 * a thread attempts to unlock a mutex that it has not locked or a 
 * mutex which is unlocked, undefined behavior results.
 *
 * If the mutex type is PTHREAD_MUTEX_ERRORCHECK, then error checking 
 * is provided. If a thread attempts to relock a mutex that it has 
 * already locked, an error will be returned. If a thread attempts 
 * to unlock a mutex that it has not locked or a mutex which is 
 * unlocked, an error will be returned.
 *
 * If the mutex type is PTHREAD_MUTEX_DEFAULT, attempting to 
 * recursively lock the mutex results in undefined behavior. 
 * Attempting to unlock the mutex if it was not locked by the 
 * calling thread results in undefined behavior. Attempting to unlock 
 * the mutex if it is not locked results in undefined behavior.
 *
 * If a signal is delivered to a thread waiting for a mutex, 
 * upon return from the signal handler the thread resumes waiting
 * for the mutex as if it was not interrupted.
 *
 *
 * When a thread owns a mutex with the PTHREAD_PRIO_NONE protocol 
 * attribute, its priority and scheduling are not affected by its 
 * mutex ownership.
 *
 * While a thread is holding a mutex that has been initialized with 
 * the PRIO_INHERIT or PRIO_PROTECT protocol attributes, it will not
 * be subject to being moved to the tail of the scheduling queue at its
 * priority in the event that its original priority is changed, such as
 * by a call to sched_setparam(). Likewise, when a thread unlocks a mutex 
 * that has been initialized with the PRIO_INHERIT or PRIO_PROTECT protocol 
 * attributes, it will not be subject to being moved to the tail of the 
 * scheduling queue at its priority in the event that its original 
 * priority is changed.
 */

static int lock(pthread_mutex_t *mutex, SceUInt *t)
{
  pthread_t me = pthread_self();
  int res;
  pthread_t oldthread = NULL;
  int oldprio = -1;

  if (!VALID(mutex)) return EINVAL;

  if (STATIC_INIT(mutex))
    {
      res = init_static(mutex);
	  if (res) return res;
    }

  if (mutex->type == PTHREAD_MUTEX_RECURSIVE)
    {
      /*
       * If the mutex type is PTHREAD_MUTEX_RECURSIVE, then the mutex 
       * maintains the concept of a lock count. When a thread successfully
       * acquires a mutex for the first time, the lock count is set to one.
       * Each time the thread relocks this mutex, the lock count is 
       * incremented by one. Each time the thread unlocks the mutex, the 
       * lock count is decremented by one. When the lock count reaches 
       * zero, the mutex becomes available for other threads to acquire. 
       * If a thread attempts to unlock a mutex that it has not locked or
       * a mutex which is unlocked, an error will be returned.
       */
      if (mutex->owner == me)
        {
          mutex->recursivecount++;
          return 0;
        }
    }

  else if (mutex->type == PTHREAD_MUTEX_ERRORCHECK)
    if (mutex->owner == me)
      return EDEADLK;
  
  if (mutex->protocol == PTHREAD_PRIO_INHERIT)
    {
     /*
       * When a thread is blocking higher priority threads because of owning 
       * one or more mutexes with the PTHREAD_PRIO_INHERIT protocol attribute, 
       * it executes at the higher of its priority or the priority of the 
       * highest priority thread waiting on any of the mutexes owned by this 
       * thread and initialized with this protocol.
       */
      // ### Critical region!
	  //int stat = sceKernelSuspendDispatchThread();
      if (mutex->owner != NULL)
        {
          /* We are about to block */
          SceKernelThreadInfo info;
          int myprio = sceKernelGetThreadCurrentPriority();
          info.size = sizeof(info);
          res = sceKernelGetThreadInfo(mutex->owner->id, &info);
          sceCHECK(res);
          if (myprio < (oldprio = info.currentPriority))
            {
              oldthread = mutex->owner;

              // Increase the priority of the thread holding the mutex.
              res = sceKernelChangeThreadPriority(mutex->owner->id, myprio);
              sceCHECK(res);
            }
        }
	  //sceKernelResumeDispatchThread(stat);
    }
  
  else if (mutex->protocol == PTHREAD_PRIO_PROTECT)
    {
      /*
       * When a thread owns one or more mutexes initialized with the 
       * PTHREAD_PRIO_PROTECT protocol, it executes at the higher of its 
       * priority or the highest of the priority ceilings of all the mutexes 
       * owned by this thread and initialized with this attribute, regardless 
       * of whether other threads are blocked on any of these mutexes.
       *
       * If a thread simultaneously owns several mutexes initialized with 
       * different protocols, it will execute at the highest of the priorities 
       * that it would have obtained by each of these protocols.
       */
      int p = sceKernelGetThreadCurrentPriority();
      me->priomutex[mutex->prioceiling]++;
      
      if (mutex->prioceiling < p) 
        {
          // Increase the priority of the thread
          res = sceKernelChangeThreadPriority(me->id, mutex->prioceiling);
          sceCHECK(res);
        }
    }

  // ### critical region.  should set owner in a callback ?
  res = sceKernelWaitSema(mutex->id, 1, t);
  if (res == SCE_OK)
    {   /* Normal condition */
    }
  else if (res == (int)SCE_KERNEL_ERROR_WAIT_TIMEOUT)
    return ETIMEDOUT;
  else 
    return EINVAL;
  

  mutex->owner = me;
  if (mutex->protocol == PTHREAD_PRIO_INHERIT)
    {
      if (oldthread != NULL && !oldthread->terminated) 
        {
          // Restore the original priority
          res = sceKernelChangeThreadPriority(oldthread->id, oldprio);
          sceCHECK(res);
        }
    }

  return 0;
}

/*
  if (res == SCE_KERNEL_ERROR_ILLEGAL_CONTEXT)
  else if (res == SCE_KERNEL_ERROR_ILLEGAL_COUNT)
  else if (res == SCE_KERNEL_ERROR_UNKNOWN_SEMID)
  else if (res == SCE_KERNEL_ERROR_SEMA_ZERO)
  else if (res == SCE_KERNEL_ERROR_WAIT_CANCEL)
  else if (res == SCE_KERNEL_ERROR_RELEASE_WAIT)
  else if (res == SCE_KERNEL_ERROR_CAN_NOT_WAIT)
  else if (res == SCE_KERNEL_ERROR_WAIT_DELETE)
  else if (res == SCE_KERNEL_ERROR_WAIT_TIMEOUT)
*/


int g_pThreadLocking = 1;
EXTERN int pthread_mutex_lock(pthread_mutex_t *mutex)
{
	if (!g_pThreadLocking)
	{
		return 0;
	}
	else
	{
		return lock(mutex, NULL);
	}	
}


EXTERN int pthread_mutex_timedlock(pthread_mutex_t *mutex, const struct timespec *abstime)
{
	if (!g_pThreadLocking)
	{
		return 0;
	}
	else
	{
		SceUInt delta;

		if (abstime == NULL)
		{
		    return EINVAL;
		}
  
		delta = getDeltaTime(abstime);

		return lock(mutex, &delta);
	}
}


/*
 * The function pthread_mutex_trylock is identical to 
 * pthread_mutex_lock except that if the mutex object referenced by 
 * the mutex parameter is currently locked (by any thread, including 
 * the current thread), the call returns immediately.
 */

EXTERN int pthread_mutex_trylock(pthread_mutex_t *mutex)
{
	if (!g_pThreadLocking)
	{
		return 0;
	}
	else
	{
	  int res;

	  if (!VALID(mutex)) return EINVAL;

	  if (STATIC_INIT(mutex))
		{
		  res = init_static(mutex);
		  if (res) return res;
		}

	  if (mutex->type == PTHREAD_MUTEX_RECURSIVE)
		{
		  if (mutex->owner == pthread_self())
			{
			  mutex->recursivecount++;
			  return 0;
			}
		}

	  res = sceKernelPollSema(mutex->id, 1);
	  if (res == SCE_OK)
		{
		  mutex->owner = pthread_self();
		  return 0;
		}
	  else if (res == (int)SCE_KERNEL_ERROR_SEMA_ZERO)
		return EBUSY;
	  else
		return EINVAL;
	}
}


/*
 * The pthread_mutex_unlock function releases the mutex object 
 * referenced by mutex. The manner in which a mutex is released is 
 * dependent upon the mutex's type attribute. If there are threads
 * blocked on the mutex object referenced by the mutex parameter 
 * when pthread_mutex_unlock is called, resulting in the mutex 
 * becoming available, the scheduling policy is used to determine
 * which thread will acquire the mutex. (In the case of 
 * PTHREAD_MUTEX_RECURSIVE mutexes, the mutex becomes available 
 * when the count reaches zero and the calling thread no longer
 * has any locks on this mutex).
 */

EXTERN int pthread_mutex_unlock(pthread_mutex_t *mutex)
{
	if (!g_pThreadLocking)
	{
		return 0;
	}
	else
	{
	  pthread_t me = pthread_self();
	  int res;
	  int i;

	  if (!VALID(mutex)) return EINVAL;

	  if (mutex->type == PTHREAD_MUTEX_RECURSIVE)
		{
		  if (mutex->owner == me)
			{
			  if (mutex->recursivecount > 0)
				{
				  mutex->recursivecount--;
				  return 0;
				}
			  else
				goto unlock;
			}
		  else
			  return EPERM;
		}

	  if (mutex->type == PTHREAD_MUTEX_ERRORCHECK)
		if (mutex->owner != me)
		  return EPERM;

	 unlock:
	  if (mutex->owner == NULL)
		return EPERM;

	  mutex->owner = NULL;
	  res = sceKernelSignalSema(mutex->id, 1);

	  if (mutex->protocol == PTHREAD_PRIO_PROTECT)
		{
		  me->priomutex[mutex->prioceiling]--;
		  // Set the thread priority to the highest priority of the 
		  // mutex owned, or the orignial priority of the thread
		  for (i = 0; i < 128; i++)
			if (i == me->priority || me->priomutex[i] > 0)
			  {
				// ### We should prevent dispatching because we are lowering 
				// our priority and we could be put in a situation where we will
				// never release the semaphore!
				res = sceKernelChangeThreadPriority(me->id, i);
				sceCHECK(res);
				break;
			  }
		}

	  else if (mutex->protocol == PTHREAD_PRIO_INHERIT) 
		{
		  /* Reset our priority to its original value in case we
		   * have been bumped to a higher priority
		   */
		  res = sceKernelChangeThreadPriority(me->id, me->priority);
		  sceCHECK(res);
		}

	  if (res == SCE_OK)
		return 0;
	  else 
		return EINVAL;
	}
}


/*
 * The pthread_mutex_getprioceiling subroutine returns the current priority 
 * ceiling of the mutex.
 * 
 * The pthread_mutex_setprioceiling subroutine either locks the mutex if it 
 * is unlocked, or blocks until it can successfully lock the mutex, then it 
 * changes the mutex's priority ceiling and releases the mutex. When the 
 * change is successful, the previous value of the priority ceiling shall 
 * be returned in old_ceiling. The process of locking the mutex need not 
 * adhere to the priority protect protocol.
 *
 * If the pthread_mutex_setprioceiling subroutine fails, the mutex priority 
 * ceiling is not changed.
 *
 * The pthread_mutex_getprioceiling and pthread_mutex_setprioceiling 
 * subroutines can fail if:
 *
 *   EINVAL     The priority requested by the prioceiling parameter is 
 *              out of range. 
 *   EINVAL     The value specified by the mutex parameter does not refer to 
 *              a currently existing mutex. 
 *   ENOSYS     This function is not supported (draft 7). 
 *   ENOTSUP    This function is not supported together with checkpoint/restart. 
 *   EPERM      The caller does not have the privilege to perform the operation. 
 */

EXTERN int pthread_mutex_getprioceiling(const pthread_mutex_t *mutex, int *prioceiling)
{
  if (!VALID(mutex)) return EINVAL;

  *prioceiling = mutex->prioceiling;
  return 0;
}


EXTERN int pthread_mutex_setprioceiling(pthread_mutex_t *mutex, int prioceiling, int *old_ceiling)
{
  int res;
  if (!VALID(mutex)) return EINVAL;

  res = sceKernelWaitSema(mutex->id, 1, NULL);
  sceCHECK(res);
  *old_ceiling = mutex->prioceiling;
  mutex->prioceiling = prioceiling;
  res = sceKernelSignalSema(mutex->id, 1);
  sceCHECK(res);
  return 0;
}


EXTERN int pthread_mutexattr_init(pthread_mutexattr_t *attr)
{
  attr->MaxCount = 1;
  attr->type = PTHREAD_MUTEX_DEFAULT;
  attr->scheduling = PTHREAD_QUEUE_FIFO_NP;
  attr->protocol = PTHREAD_PRIO_NONE;
  attr->prioceiling = SCE_KERNEL_PROCESS_PRIORITY_USER_LOW;
  return 0;
}


EXTERN int pthread_mutexattr_destroy(pthread_mutexattr_t *attr)
{
  (void)&attr;
  return 0;
}


/*
 * The pthread_mutexattr_setprotocol() and pthread_mutexattr_getprotocol()
 * functions, respectively, set and get the protocol attribute of a mutex
 * attribute object pointed to by attr, which was previously created by 
 * the pthread_mutexattr_init() function. 
 *
 * The protocol attribute defines the protocol to be followed in utilizing
 * mutexes. The value of protocol may be one of PTHREAD_PRIO_NONE, 
 * PTHREAD_PRIO_INHERIT, or PTHREAD_PRIO_PROTECT.
 */

EXTERN int pthread_mutexattr_getprotocol(const pthread_mutexattr_t *attr, int *protocol)
{
  *protocol = attr->protocol;
  return 0;
}


EXTERN int pthread_mutexattr_setprotocol(pthread_mutexattr_t *attr, int protocol)
{
  if (protocol != PTHREAD_PRIO_NONE && 
      protocol != PTHREAD_PRIO_INHERIT && 
      protocol != PTHREAD_PRIO_PROTECT)
    return ENOTSUP;

  attr->protocol = protocol;
  return 0;
}


EXTERN int pthread_mutexattr_gettype(const pthread_mutexattr_t *attr, int *type)
{
  *type = attr->type;
  return 0;
}


EXTERN int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type)
{
  if (type != PTHREAD_MUTEX_NORMAL &&
      type != PTHREAD_MUTEX_ERRORCHECK &&
      type != PTHREAD_MUTEX_RECURSIVE)
    return ENOTSUP;

  attr->type = type;
  return 0;
}


/*
 * Gets and sets the prioceiling attribute of the mutex attributes object.
 *
 * The pthread_mutexattr_getprioceiling and pthread_mutexattr_setprioceiling 
 * subroutines can fail if:
 *
 *   EINVAL     The value specified by the attr or prioceiling parameter is 
 *              invalid. 
 *   ENOSYS     This function is not supported (draft 7). 
 *   ENOTSUP    This function is not supported together with checkpoint/restart. 
 *   EPERM      The caller does not have the privilege to perform the operation. 
 */

EXTERN int pthread_mutexattr_getprioceiling(const pthread_mutexattr_t *attr, int *prioceiling)
{
  *prioceiling = attr->prioceiling;
  return 0;
}


EXTERN int pthread_mutexattr_setprioceiling(pthread_mutexattr_t *attr, int prioceiling)
{
  attr->prioceiling = prioceiling;
  return 0;
}


EXTERN int pthread_mutexattr_getqueueingpolicy_np(const pthread_mutexattr_t *attr, int *policy)
{
  *policy = attr->scheduling;
  return 0;
}


EXTERN int pthread_mutexattr_setqueueingpolicy_np(pthread_mutexattr_t *attr, int policy)
{
  if (policy != PTHREAD_QUEUE_FIFO_NP && policy != PTHREAD_QUEUE_PRIORITY_NP)
    return EINVAL;
  attr->scheduling = policy;
  return 0;
}
