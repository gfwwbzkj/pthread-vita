//Sony Computer Entertainment Confidential
#include "pthread/include/pthread.h"


static void cleanup(void *p)
{
  pthread_once_t *o = (pthread_once_t *)p;
  // Do not set o->done because a canceled init is not
  // an init!
  pthread_mutex_unlock(&o->in_progress);
}


/*
 * The pthread_once subroutine executes the routine 
 * init routine exactly once in a process. The first 
 * call to this subroutine by any thread in the process 
 * executes the given routine, without parameters. Any 
 * subsequent call will have no effect.
 */

EXTERN int pthread_once(pthread_once_t *once_control, void (*init)(void))
{
  PTHREAD_INIT();

  if (once_control == NULL || init == NULL)
     return EINVAL;

  if (once_control->done)
    return 0;

  pthread_mutex_lock(&once_control->in_progress);
  if (! once_control->done)
    {
      /* We need to register a cleanup function in case the thread
	 is canceled during the 'once part'.  
      */
      pthread_cleanup_push(cleanup, once_control);
      init();
      pthread_cleanup_pop(0);
      once_control->done = 1;
    }
  pthread_mutex_unlock(&once_control->in_progress);
  return 0;
}
