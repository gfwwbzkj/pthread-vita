//Sony Computer Entertainment Confidential
#include "pthread/include/pthread.h"

#if 0
/* Common definitions */
#define VALID(mbx) \
	(((mbx) != 0) && ((mbx)->id != INVALID_ID_))
#define INVALIDATE(mbx) \
	do { (mbx)->id = INVALID_ID_; } while(0)


EXTERN int pthread_mbx_destroy_np(pthread_mbx_t *mbx)
{
  int res;

  if (!VALID(mbx)) return EINVAL;

  ENTER_CRITICAL();
  res = sceKernelDeleteMbx(mbx->id);
  sceCHECK(res);
  
  INVALIDATE(mbx);
  LEAVE_CRITICAL();
  return 0;
}


EXTERN int pthread_mbx_init_np(pthread_mbx_t *restrict mbx, 
                               const pthread_mbxattr_t *restrict attr)
{
  pthread_mbxattr_t a;
  int res;

  CHECK_PT_PTR(mbx);
  ENTER_CRITICAL();
  
  if (attr == NULL)
    pthread_mbxattr_init_np(&a);
  else
    a = *attr;

  res = sceKernelCreateMbx("pthread mailbox", a.qwait | a.qmsg, NULL);
  if (res > 0)
    {
      mbx->id = res;
      res = 0;
    }

  LEAVE_CRITICAL();
  return ERROR_errno_sce(res); 
}


EXTERN int pthread_mbx_send_np(pthread_mbx_t *mbx, 
			       pthread_mbxmsg_t *msg)
{
  int res;

  if (!VALID(mbx)) return EINVAL;

  res = sceKernelSendMbx(mbx->id, (SceKernelMsgPacket *)msg);
  sceCHECK(res);

  return ERROR_errno_sce(res);
}


static int receive(pthread_mbx_t *mbx,
		   pthread_mbxmsg_t **msg,
		   SceUInt *t)
{
  int res;

  if (!VALID(mbx)) return EINVAL;

  res = sceKernelReceiveMbx(mbx->id, (SceKernelMsgPacket **)msg, t);
  if (res == SCE_OK)
    {   /* Normal condition */
      return 0;
    }
  else if (res == (int)SCE_KERNEL_ERROR_WAIT_TIMEOUT)
    return ETIMEDOUT;
  else 
    return EINVAL;
}


EXTERN int pthread_mbx_receive_np(pthread_mbx_t *mbx,
				  pthread_mbxmsg_t **msg)
{
  return receive(mbx, msg, NULL);
}


EXTERN int pthread_mbx_timedreceive_np(pthread_mbx_t *mbx,
				       pthread_mbxmsg_t **msg,
				       const struct timespec *abstime)
{
  SceUInt delta;
  
  if (abstime == NULL)
    return EINVAL;
  
  delta = getDeltaTime(abstime);

  return receive(mbx, msg, &delta);
}


EXTERN int pthread_mbxattr_init_np(pthread_mbxattr_t *attr)
{
  attr->qwait = SCE_KERNEL_MBA_THPRI;
  attr->qmsg = SCE_KERNEL_MBA_MSPRI;
  return 0;
}


EXTERN int pthread_mbxattr_destroy_np(pthread_mbxattr_t *attr)
{
  (void)&attr;
  return 0;
}

#endif