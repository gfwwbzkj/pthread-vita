//Sony Computer Entertainment Confidential
#include "pthread/include/pthread.h"

EXTERN int pthread_num_processors_np()
{
  return 1;
}


/* 
 * Put the current thread in WAIT state for the specified amount of 
 * micro second
 */

EXTERN int pthread_sleep_np(int usecond)
{
  pthread_testcancel();
  sceKernelDelayThread(usecond);
  pthread_testcancel();
  return 0;
}


EXTERN int pthread_delay_np(const struct timespec *time)
{
  return pthread_sleep_np(timespec2usec(time)); 
}


EXTERN int pthread_delay_until_np(const struct timespec *abstime)
{
  struct timespec now;
  long long a, n;
  int usec;

  pthread_getsystemtime_np(&now);
  a = timespec2usec(abstime);
  n = timespec2usec(&now);
  usec = a - n;
  if (usec < 0)
    usec = 0;
  return pthread_sleep_np(usec);
}


EXTERN void *pthread_timechange_handler_np (void *arg)
{
  int res = 0;
  //  pthread_cond_t c;

  // Unused parameters
  (void)&arg;

  // ### Not implemented yet
  return (void *) (res != 0 ? EAGAIN : 0);
}


EXTERN int pthread_getsystemtime_np(struct timespec *t)
{
  SceKernelSysClock c;
  sceKernelGetProcessTime(&c);
  // sceKernelSysClock2USec(&c, &sec, &usec);

  double time_sec = ((double)c.quad / 1000000.0); 

  t->tv_sec = (int)time_sec;
  t->tv_nsec = (int)((time_sec - (double)t->tv_sec) * 1000000.0);

  return 0;
}


EXTERN int pthread_getthreadid_np(pthread_t thread, int *id)
{
  if (thread == NULL)
    return EINVAL;

  *id = thread->id;
  return 0;
}
