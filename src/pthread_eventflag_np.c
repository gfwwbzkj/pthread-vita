//Sony Computer Entertainment Confidential
#include "pthread/include/pthread.h"

/* Common definitions */
#define VALID(evtflag) \
	(((evtflag) != 0) && ((evtflag)->id != INVALID_ID_))
#define INVALIDATE(evtflag) \
	do { (evtflag)->id = INVALID_ID_; } while (0)


EXTERN int pthread_eventflag_destroy_np(pthread_eventflag_t *evtflag)
{
  int res;

  if (!VALID(evtflag)) return EINVAL;

  ENTER_CRITICAL();
  res = sceKernelDeleteEventFlag(evtflag->id);
  sceCHECK(res);

  INVALIDATE(evtflag);
  LEAVE_CRITICAL();
  return 0;
}


EXTERN int pthread_eventflag_init_np(pthread_eventflag_t *restrict evtflag,
                                     const pthread_eventflagattr_t *restrict attr)
{
  pthread_eventflagattr_t a;
  int res;

  CHECK_PT_PTR(evtflag);

  ENTER_CRITICAL();

  if (attr == NULL)
    pthread_eventflagattr_init_np(&a);
  else
    a = *attr;

  res = sceKernelCreateEventFlag("pthread event flag", a.attr, a.init, NULL);
  if (res > 0)
    {
      evtflag->id = res;
      res = 0;
    }
  else
    {
	  sceCHECK(res);
	  INVALIDATE(evtflag);
      res = ERROR_errno_sce(res);
    }

  LEAVE_CRITICAL();
  return res;
}


EXTERN int pthread_eventflag_set_np(pthread_eventflag_t *evtflag, int pattern)
{
  int res;

  if (!VALID(evtflag)) return EINVAL;

  res = sceKernelSetEventFlag(evtflag->id, pattern);
  sceCHECK(res);
  return ERROR_errno_sce(res);
}


static int wait(pthread_eventflag_t *evtflag, 
                unsigned int pattern,
                int waitmode,
                int clearmode,
                SceUInt *abstime,
                unsigned int *result)
{
  int res;
  unsigned int result_ = 0;

  if (!VALID(evtflag)) return EINVAL;

  res = sceKernelWaitEventFlag(evtflag->id, pattern, waitmode | clearmode, &result_, abstime);
  if (result != NULL)
    *result = result_;
  if (res == SCE_OK)
    {   /* Normal condition */
    }
  else if (res == (int)SCE_KERNEL_ERROR_WAIT_TIMEOUT)
    return ETIMEDOUT;
  else 
    return EINVAL;
  return 0;
}


EXTERN int pthread_eventflag_wait_np(pthread_eventflag_t *evtflag, 
                                     unsigned int pattern,
                                     int waitmode,
                                     int clearmode,
                                     unsigned int *result)
{
  return wait(evtflag, pattern, waitmode, clearmode, NULL, result);
}


EXTERN int pthread_eventflag_try_np(pthread_eventflag_t *evtflag, 
                                    unsigned int pattern,
                                    int waitmode,
                                    int clearmode,
                                    unsigned int *result)
{
  int res;

  if (!VALID(evtflag)) return EINVAL;
  res = sceKernelPollEventFlag(evtflag->id, pattern, waitmode | clearmode, result);
  sceCHECK(res);
  return 0;
}


EXTERN int pthread_eventflag_timedwait_np(pthread_eventflag_t *evtflag, 
                                          unsigned int pattern,
                                          int waitmode,
                                          int clearmode,
                                          const struct timespec *abstime,
                                          unsigned int *result)
{
  SceUInt delta;

  CHECK_PT_PTR(abstime);
  
  delta = getDeltaTime(abstime);
  return wait(evtflag, pattern, waitmode, clearmode, &delta, result);
}


EXTERN int pthread_eventflagattr_init_np(pthread_eventflagattr_t *attr)
{
  attr->attr = SCE_KERNEL_EVF_ATTR_MULTI;
  attr->init = 0;
  return 0;
}


EXTERN int pthread_eventflagattr_destroy_np(pthread_eventflagattr_t *attr)
{
  (void)&attr;
  return 0;
}

