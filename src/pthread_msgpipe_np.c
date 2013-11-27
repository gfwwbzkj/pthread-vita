//Sony Computer Entertainment Confidential
#include "pthread/include/pthread.h"

/* Common definitions */
#define VALID(msgpipe) \
	(((msgpipe) != 0) && ((msgpipe)->id != INVALID_ID_))
#define INVALIDATE(msgpipe) \
	do { (msgpipe)->id = INVALID_ID_; } while(0)


EXTERN int pthread_msgpipe_destroy_np(pthread_msgpipe_t *msgpipe)
{
  return EINVAL;
}


EXTERN int pthread_msgpipe_init_np(pthread_msgpipe_t *restrict msgpipe,
                                   const pthread_msgpipeattr_t *restrict attr)
{
  return EINVAL;
}


EXTERN int pthread_msgpipe_send_np(pthread_msgpipe_t *msgpipe, 
                                   void *buffer,
                                   int size,
                                   int waitmode,
                                   int *result)
{
  return EINVAL;
}


EXTERN int pthread_msgpipe_trysend_np(pthread_msgpipe_t *msgpipe, 
                                      void *buffer,
                                      int size,
                                      int waitmode,
                                      int *result)
{
  return EINVAL;
}


EXTERN int pthread_msgpipe_timedsend_np(pthread_msgpipe_t *msgpipe, 
                                        void *buffer,
                                        int size,
                                        int waitmode,
                                        const struct timespec *abstime,
                                        int *result)
{
    return EINVAL;
}


EXTERN int pthread_msgpipe_receive_np(pthread_msgpipe_t *msgpipe, 
                                      void *buffer,
                                      int size,
                                      int waitmode,
                                      int *result)
{
    return EINVAL;
}


EXTERN int pthread_msgpipe_tryreceive_np(pthread_msgpipe_t *msgpipe, 
                                         void *buffer,
                                         int size,
                                         int waitmode,
                                         int *result)
{
    return EINVAL;
}


EXTERN int pthread_msgpipe_timedreceive_np(pthread_msgpipe_t *msgpipe, 
                                           void *buffer,
                                           int size,
                                           int waitmode,
                                           const struct timespec *abstime,
                                           int *result)
{
    return EINVAL;
}


EXTERN int pthread_msgpipeattr_init_np(pthread_msgpipeattr_t *attr)
{
    return EINVAL;
}


EXTERN int pthread_msgpipeattr_destroy_np(pthread_msgpipeattr_t *attr)
{
    return EINVAL;
}

