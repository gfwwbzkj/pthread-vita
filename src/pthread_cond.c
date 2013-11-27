//Sony Computer Entertainment Confidential
#include "pthread/include/pthread.h"

/* Common definitions */
#define VALID(cond) \
	(((cond) != 0) && ((cond)->control != INVALID_ID_))
#define INVALIDATE(cond) \
	do { (cond)->control = INVALID_ID_; } while(0)
#define STATIC_INIT(cond) \
	(((cond)->control == STATIC_INIT_ID_) && ((cond)->NbWait == COND_SIG_))


typedef struct
{
  pthread_t	      me;
  pthread_cond_t  *cond;
  pthread_mutex_t *mutex;
  int             recursivecount;
} callback_param_t;

static int CallbackHandler(SceUID notifyId, int count, int arg, void *common)
{
  int rc, i;

  callback_param_t *cb = (callback_param_t *)arg;
  pthread_t me = cb->me;

  TRACE(cb->mutex, cb->mutex->id, "unlocking cond mutex");

  // Unused parameters
  (void)&count;
  (void)&common;
  (void)&notifyId;

  cb->recursivecount = cb->mutex->recursivecount;
  
  for (i=0; i<=cb->recursivecount; ++i)
    {
	  rc = pthread_mutex_unlock(cb->mutex);
	  pCHECK(rc);
	}

  UNLOCK_CONTROL(cb->cond);

  me->inWAIT = 1;
  return 0;
}


EXTERN int pthread_cond_init(pthread_cond_t *cond,
                             const pthread_condattr_t *attr)
{
  int rc;

  // Unused parameters
  (void)&attr;
  
  CHECK_PT_PTR(cond);
  
  ENTER_CRITICAL();

  rc = sceKernelCreateSema("pthread cond",
			   SCE_KERNEL_ATTR_TH_FIFO,
			   0, 0x7fffffff, NULL);
  if (rc > 0) 
    {
      cond->lock = rc;
      cond->NbWait = 0;
	  rc = 0;
      INIT_CONTROL(cond);
    }
  else
    {
      sceCHECK(rc);
	  rc = ERROR_errno_sce(rc);
	  if (!STATIC_INIT(cond))
		  INVALIDATE(cond);
    }

  LEAVE_CRITICAL();
  return rc;
}


EXTERN int pthread_cond_destroy(pthread_cond_t *cond)
{
  int rc, res = 0;

  if (!VALID(cond)) return EINVAL;

  ENTER_CRITICAL();
  if (!STATIC_INIT(cond))
    {
      if (cond->NbWait > 0)
          res = EBUSY;
      else
        {
          DELETE_CONTROL(cond);

          rc = sceKernelDeleteSema(cond->lock);
          sceCHECK(rc);

          INVALIDATE(cond);
        }
    }
  LEAVE_CRITICAL();
  return res;
}


static int init_static(pthread_cond_t *cond)
{
  int res = 0;
  ENTER_CRITICAL();

  if (STATIC_INIT(cond))
    res = pthread_cond_init(cond, NULL);

  LEAVE_CRITICAL();
  return res;
}


typedef struct {
  pthread_mutex_t *mutex;
  int             *result;
  int             *recursivecount;
} cleanup_t;

static void cleanup(void *arg)
{
  cleanup_t *c = (cleanup_t *)arg;
  pthread_mutex_t *mutex = c->mutex;
  int res, i;
  
  TRACE(cond->mutex, c->mutex->id, "relocking cond mutex");

  for (i=0; i<=*c->recursivecount; ++i)
    if ((res = pthread_mutex_lock(mutex)) != 0)
      *c->result = res;
}


/*
 * The pthread_cond_wait and pthread_cond_timedwait functions 
 * are used to block on a condition variable. They are called 
 * with mutex locked by the calling thread or undefined 
 * behavior will result.
 *
 * These functions atomically release mutex and cause the 
 * calling thread to block on the condition variable cond; 
 * atomically here means atomically with respect to access by 
 * another thread to the mutex and then the condition variable. 
 * That is, if another thread is able to acquire the mutex 
 * after the about-to-block thread has released it, then a 
 * subsequent call to pthread_cond_signal or 
 * pthread_cond_broadcast in that thread behaves as if it were 
 * issued after the about-to-block thread has blocked.
 * 
 * Upon successful return, the mutex is locked and owned by the 
 * calling thread
 */

static int lock(pthread_cond_t *cond, pthread_mutex_t *mutex, SceUInt *time)
{
  callback_param_t cb;
  int res, returncode;
  pthread_t me = pthread_self();
  cleanup_t cleanarg;

  if (!VALID(cond)) return EINVAL;

  PTHREAD_INIT();

  if (STATIC_INIT(cond))
    {
      res = init_static(cond);
	  if (res) return res;
    }

  LOCK_CONTROL(cond);
  cb.cond = cond;
  cb.mutex = mutex;
  cb.me = me;
  cb.recursivecount = 0;

  if (me->condCBID == INVALID_ID_)
    {
      res = sceKernelCreateCallback("cond callback", 0, CallbackHandler, NULL);
      if (res > 0)
        me->condCBID = res;
      else
        sceCHECK(res);
    }

  // Prepare the cleanup callback
  cleanarg.mutex = mutex;
  cleanarg.result = &returncode;
  cleanarg.recursivecount = &cb.recursivecount;
  pthread_cleanup_push(cleanup, (void *)&cleanarg);

  TRACE(cond, cond[0]->lock, ">cond WaitSemaCB");
  cond->NbWait++;

  res = sceKernelNotifyCallback(me->condCBID, (int)&cb);
  sceCHECK(res);

  returncode = 0;
  res = sceKernelWaitSemaCB(cond->lock, 1, time);
  /*
   * Check if the thread has been canceled during a wait
   * If we have been canceled we can loop forever 
   * because another thread is killing us!
   *
   */
  LOCK_CONTROL(me);
  me->inWAIT = 0;
  while (me->id == DELETED_ID_)
    sceKernelDelayThread(1000000); 
  UNLOCK_CONTROL(me);              
  
  if (res == (int)SCE_KERNEL_ERROR_WAIT_TIMEOUT)
    {
      returncode = ETIMEDOUT;
      LOCK_CONTROL(cond);
      cond->NbWait--;
      UNLOCK_CONTROL(cond);
    }

  pthread_cleanup_pop(1);
  return returncode;
}


EXTERN int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
  return lock(cond, mutex, NULL);
}


EXTERN int pthread_cond_timedwait(pthread_cond_t *cond,
				  pthread_mutex_t *mutex, 
				  const timespec *abstime)
{
  SceUInt delta;

  if (abstime == NULL)
    return EINVAL;

  delta = getDeltaTime(abstime);

  return lock(cond, mutex, &delta);
}


/*
 * The pthread_cond_signal subroutine unblocks at least one 
 * blocked thread, while the pthread_cond_broadcast subroutine 
 * unblocks all the blocked threads.
 *
 * The pthread_cond_signal or pthread_cond_broadcast functions
 * may be called by a thread whether or not it currently owns the
 * mutex that threads calling pthread_cond_wait or 
 * pthread_cond_timedwait have associated with the condition 
 * variable during their waits; however, if predictable scheduling 
 * behavior is required, then that mutex is locked by the thread 
 * calling pthread_cond_signal or pthread_cond_broadcast.
 *
 */

EXTERN int pthread_cond_signal(pthread_cond_t *cond)
{
  int res;

  if (!VALID(cond)) return EINVAL;

  if (STATIC_INIT(cond) || cond->NbWait == 0)
    return 0;

  LOCK_CONTROL(cond);
  
  if (cond->NbWait > 0) 
    {
      res = sceKernelSignalSema(cond->lock, 1);
      sceCHECK(res);
      cond->NbWait--;
    }

  UNLOCK_CONTROL(cond);

  return 0;
}


EXTERN int pthread_cond_broadcast(pthread_cond_t *cond)
{
  int res;
//printf("Bcast - 0\n");

  if (!VALID(cond)) return EINVAL;

  if (STATIC_INIT(cond) || cond->NbWait == 0)
    return 0;

  LOCK_CONTROL(cond);
//printf("Bcast - 1\n");

  if (cond->NbWait > 0)
    {
//printf("Bcast - 2\n");
      res = sceKernelSignalSema(cond->lock, cond->NbWait);
//printf("Bcast - 3\n");
      sceCHECK(res);
      cond->NbWait = 0;
    }
//printf("Bcast - 4\n");

  UNLOCK_CONTROL(cond);
//printf("Bcast - 5\n");

  return 0;
}


EXTERN int pthread_condattr_init(pthread_condattr_t *attr)
{
  // Unused parameters
  (void)&attr;
  return 0;
}


EXTERN int pthread_condattr_destroy(pthread_condattr_t *attr)
{
  // Unused parameters
  (void)&attr;
  return 0;
}


//EXTERN int pthread_condattr_getclock(const pthread_condattr_t *attr,
//                                   clockid_t *c)
//{
//}


//EXTERN int pthread_condattr_setclock(pthread_condattr_t *attr, clockid_t c)
//{
//}



