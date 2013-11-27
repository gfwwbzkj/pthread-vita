//Sony Computer Entertainment Confidential
#include "pthread/include/pthread.h"

/*
 * The sched_yield() function forces the running thread to relinquish 
 * the processor until it again becomes the head of its thread list. 
 * It takes no arguments. 
 *
 * The sched_yield() function returns 0 if it completes successfully,
 * or it returns a value of -1 and sets errno to indicate the error. 
 *
 * sched_yield is usually declared in <sched.h> but it is part of the POSIX.1b
 * spec (realtime extensions)
 */

EXTERN int sched_yield(void)
{
  // sceKernelRotateThreadReadyQueue(SCE_KERNEL_TPRI_RUN);
  return 0;
}

