//Sony Computer Entertainment Confidential
#include "pthread/include/pthread.h"

/* Common definitions */
#define VALID(barrier) \
	(((barrier) != 0) && ((barrier)->control != INVALID_ID_))
#define INVALIDATE(barrier) \
	do { (barrier)->control = INVALID_ID_; } while(0)

#if 0
static int CallbackHandler(SceUID notifyId, int count, int arg, void *common)
{
  pthread_barrier_t *b = (pthread_barrier_t*)arg;

  // Unused parameters
  (void)&count;
  (void)&common;
  (void)&notifyId;

  // Release the barrier control
  UNLOCK_CONTROL(b);
  return 0;
}
#endif

/*
 * The pthread_barrier_destroy subroutine destroys the 
 * barrier referenced by the barrier parameter and releases 
 * any resources used by the barrier. The effect of subsequent 
 * use of the barrier is undefined until the barrier is 
 * reinitialized by another call to the pthread_barrier_init 
 * subroutine. An implementation can use this subroutine to set 
 * the barrier parameter to an invalid value. The results are 
 * undefined if the pthread_barrier_destroy subroutine is called 
 * when any thread is blocked on the barrier, or if this function 
 * is called with an uninitialized barrier.
 *
 * The pthread_barrier_init subroutine allocates any resources 
 * required to use the barrier referenced by the barrier parameter 
 * and initializes the barrier with attributes referenced by the 
 * attr parameter. If the attr parameter is NULL, the default 
 * barrier attributes are used; the effect is the same as passing 
 * the address of a default barrier attributes object. The results 
 * are undefined if pthread_barrier_init subroutine is called when
 * any thread is blocked on the barrier (that is, has not returned
 * from the pthread_barrier_wait call). The results are undefined
 * if a barrier is used without first being initialized. The 
 * results are undefined if the pthread_barrier_init subroutine 
 * is called specifying an already initialized barrier.
 *
 * The count argument specifies the number of threads that must 
 * call the pthread_barrier_wait subroutine before any of them 
 * successfully return from the call. The value specified by the 
 * count parameter must be greater than zero.
 */

EXTERN int pthread_barrier_destroy(pthread_barrier_t *barrier)
{
  int rc, res = 0;

  if (!VALID(barrier)) return EINVAL;

  ENTER_CRITICAL();

  DELETE_CONTROL(barrier);

  rc = sceKernelDeleteSema(barrier->queue);
  if (rc)
    {
      sceCHECK(rc);
	  res = ERROR_errno_sce(rc);
    }
  else
    INVALIDATE(barrier);

  LEAVE_CRITICAL();
  return res;
}


EXTERN int pthread_barrier_init(pthread_barrier_t *barrier,
                                const pthread_barrierattr_t *barrierattr, 
                                unsigned int count)
{
  int rc;

  // Unused parameters
  (void)&barrierattr;

  PTHREAD_INIT();

  if (count < 1 || barrier == NULL)
    return EINVAL;

  ENTER_CRITICAL();
  
  rc = sceKernelCreateSema("pthread barrier (c)",
			   SCE_KERNEL_ATTR_TH_FIFO,
			   1, 0x7fffffff, NULL);
  if (rc > 0) 
    {
      barrier->control = rc;
      TRACE(barrier, barrier->control, "barrier->control");
  
      rc = sceKernelCreateSema("pthread barrier (q)",
                   SCE_KERNEL_ATTR_TH_FIFO,
                   0, count, NULL);
      if (rc > 0) 
	    {
          barrier->queue = rc;
          TRACE(barrier, barrier->queue, "barrier->queue");
  
          barrier->neededcount = count;
          barrier->count = 0;
		  rc = 0;
        }
	  else
	    {
          sceCHECK(rc);
		  sceKernelDeleteSema(barrier->control);
		  INVALIDATE(barrier);
        }
    }
  else
    {
      sceCHECK(rc);
	  INVALIDATE(barrier);
    }

  LEAVE_CRITICAL();
  return ERROR_errno_sce(rc);
}


EXTERN int pthread_barrier_wait(pthread_barrier_t *barrier)
{
	int res;
	pthread_t me = pthread_self();

	if (!VALID(barrier)) return EINVAL;
	LOCK_CONTROL(barrier);

	barrier->count += 1;

//	int myCount = barrier->count;

	if(barrier->count < barrier->neededcount)
	{
		UNLOCK_CONTROL(barrier);
		res = sceKernelWaitSema(barrier->queue, 1, NULL);
		sceCHECK(res);
		return 0;
	}
	else
	{
		barrier->count = 0;
		if(barrier->neededcount > 1)
		{
			res = sceKernelSignalSema(barrier->queue, barrier->neededcount - 1);
			sceCHECK(res);
		}
		UNLOCK_CONTROL(barrier);
		return PTHREAD_BARRIER_SERIAL_THREAD;
	}
}


EXTERN int pthread_barrierattr_destroy(pthread_barrierattr_t *barrierattr)
{
  // Unused parameters
  (void)&barrierattr;
  return 0;
}


EXTERN int pthread_barrierattr_init(pthread_barrierattr_t *barrierattr)
{
  // Unused parameters
  (void)&barrierattr;
  return 0;
}
