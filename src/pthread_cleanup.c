//Sony Computer Entertainment Confidential
#include "pthread/include/pthread.h"

/*
 * The pthread_cleanup_push subroutine pushes the specified
 * cancellation cleanup handler routine onto the calling 
 * thread's cancellation cleanup stack. The cancellation
 * cleanup handler is popped from the cancellation cleanup 
 * stack and invoked with the argument arg when: (a) the 
 * thread exits (that is, calls pthread_exit, (b) the thread 
 * acts upon a cancellation request, or (c) the thread calls 
 * pthread_cleanup_pop with a nonzero execute argument.
 *
 * The pthread_cleanup_pop subroutine removes the subroutine 
 * at the top of the calling thread's cancellation cleanup 
 * stack and optionally invokes it (if execute is nonzero).
 */

EXTERN void pthread_cleanup_push_(void (*routine)(void *), void *arg, pthread_cleanup_t *cleanup)
{
  pthread_t th = pthread_self();

  cleanup->func = routine;
  cleanup->arg = arg;
  cleanup->next = th->cleanup;
  th->cleanup = cleanup;
}

EXTERN void pthread_cleanup_pop_(int execute)
{
  pthread_cleanup_t *clfunc;
  pthread_t th = pthread_self();

  if ((clfunc = th->cleanup) == NULL)
    return;
  th->cleanup = clfunc->next;

  if (execute)
    clfunc->func(clfunc->arg);
}
