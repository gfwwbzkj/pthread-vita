//Sony Computer Entertainment Confidential
#ifndef _H_sched_psp
#define _H_sched_psp

#ifdef PRX_EXPORTS
#	define PTHREAD_EXPORT __declspec(dllexport)
#else
#	define PTHREAD_EXPORT
#endif

#if defined(__cplusplus)
#define EXTERN extern "C" PTHREAD_EXPORT
#else
#define EXTERN extern PTHREAD_EXPORT
#endif


enum {
  SCHED_OTHER,
  SCHED_FIFO,
  SCHED_RR,
};

/*
 * The pthread_attr_getschedparam subroutine returns the value of the 
 * schedparam attribute of the thread attributes object attr. The 
 * schedparam attribute specifies the scheduling parameters of a thread 
 * created with this attributes object. The sched_priority field of the 
 * sched_param structure contains the priority of the thread. It is an 
 * integer value
 *
 * The pthread_attr_setschedparam subroutine sets the value of the 
 * schedparam attribute of the thread attributes object attr. The 
 * schedparam attribute specifies the scheduling parameters of a thread 
 * created with this attributes object. The sched_priority field of the
 * sched_param structure contains the priority of the thread.
 */

typedef struct sched_param sched_param;
struct sched_param
{
  int sched_priority;		// Current priority
};


/*
 * The sched_yield() function forces the running thread to relinquish 
 * the processor until it again becomes the head of its thread list. 
 * It takes no arguments. 
 *
 * The sched_yield() function returns 0 if it completes successfully,
 * or it returns a value of -1 and sets errno to indicate the error. 
 *
 * sched_yield is usually declared in <sched.h> but it is part of 
 * the POSIX.1b spec (realtime extensions)
 */

EXTERN int sched_yield(void);

#endif /* _H_sched_psp */
