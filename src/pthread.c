//Sony Computer Entertainment Confidential
#include "pthread/include/pthread.h"
#include <stdarg.h>
#include <string.h> // memset
#include <moduleinfo.h>
#include <kernel/threadmgr_mono.h>	// special sony provided thread manager, downloaded using PSP2Player.pm

SCE_MODULE_INFO (PTHREAD_PRX, SCE_MODULE_ATTR_NONE, 1, 1);


static pthread_callback_t pthread_add_thread_callback = NULL;
static pthread_callback_t pthread_delete_thread_callback = NULL;

static pthread_storage_t UserMainThreadStorage;
static pthread_t UserMainThread = NULL;

/* 
 * The enter/leave critical function pointers will be set after the 
 * pthread initialization is completed, othewise we could have 
 * a deadlock situation.  During initialization we only have one 
 * thread running and the critical functions can be dummied out.
 */

static void enter_critical_func_dummy()
{
  return;
}

static void leave_critical_func_dummy()
{
  return;
}

void(* pthread_enter_critical)(void) = enter_critical_func_dummy;
void(* pthread_leave_critical)(void) = leave_critical_func_dummy;


/*
 * The real functions
 */

static void enter_critical_func()
{
  pthread_mutex_lock(&_PTGLOBAL_);
}

static void leave_critical_func()
{
  pthread_mutex_unlock(&_PTGLOBAL_);
}


static void cleanup(pthread_t th)
{
  while (th->cleanup != NULL) 
    {
      pthread_cleanup_pop_(1);
    }
}


/*
 * Cleanup all the thread data
 */

static void detach(pthread_t th, int me)
{
  int res;
  int id;
  unsigned int NumThreads = 0;

  id = th->id;
  th->id = DELETED_ID_;

  if (id != DELETED_ID_ && !me)
    {
      res = sceKernelDeleteThread(id);
      sceCHECK(res);
    }
  pthread_cleanupspecific_(th);
  res = sceKernelCancelSema(th->control, -1, &NumThreads);
  sceCHECK(res);
  res = sceKernelDeleteSema(th->control);
  sceCHECK(res);
  if (th->needsfree)
    PTHREAD_FREE(th);
}


/* 
 * Thread Event handler
 */
/*
static int ThreadEventHandler(int type, SceUID thid, void *common)
{
  pthread_t th;

  if (type == SCE_KERNEL_TE_CREATE)
    {
      pthread_printf_np("CREATE handler\n");
    }
  else if (type == SCE_KERNEL_TE_START)
    {
      pthread_printf_np("START handler\n");
    }
  else if (type == SCE_KERNEL_TE_EXIT)
    {
      th = pthread_get(thid);
      pthread_printf_np("EXIT handler %x %x\n", th, th->id);
    }
  else if (type == SCE_KERNEL_TE_DELETE)
    {
      pthread_printf_np("DELETE handler\n");
    }

  return 0;
}
*/

/*
 * sce threads and Posix thread have a different prototype.
 * The purpose of the sceGlue function is to convert between
 * the Sony prototype and the pthread prototype
 */

typedef struct 
{
  void *(*start)(void *);
  void *param;
  pthread_t thread;
} sceGlueParam_t;

EXTERN void pthread_ext_set_add_thread_callback(pthread_callback_t callback)
{
	pthread_add_thread_callback = callback;
}

EXTERN void pthread_ext_set_delete_thread_callback(pthread_callback_t callback)
{
	pthread_delete_thread_callback = callback;
}

// #ifdef SN_TARGET_PSP2
// void GC_psp2_add_thread (SceUID threadId);
// void GC_psp2_delete_thread (SceUID threadId);
// #endif


static inline int sceGlue(SceSize s, void *param)
{
  sceGlueParam_t *p = param;
  pthread_t me = p->thread;

  me->returncode = (void *)0;
  
  // Unused parameters...
  (void)&s;

  // GC_psp2_add_thread (sceKernelGetThreadId ());
  if (pthread_add_thread_callback)
  {
	  pthread_add_thread_callback(sceKernelGetThreadId ());
  }

  // Pass the control to the thread with the argument pointer
  me->returncode = p->start(p->param);
  me->terminated = 1;

  // GC_psp2_delete_thread (sceKernelGetThreadId ());
  if (pthread_delete_thread_callback)
  {
	  pthread_delete_thread_callback(sceKernelGetThreadId ());
  }

  if (me->detached) 
  {
      detach(me, 1);
  }
  return 0;
}


/*
 * The pthread_self subroutine returns the calling thread's ID.
 */

EXTERN pthread_t pthread_self(void)
{
  SceUID id;

  PTHREAD_INIT();

  id = sceKernelGetThreadId();
  if (id <= 0)
    sceCHECK(id);
  if (UserMainThread != NULL && UserMainThread->id == id)
    return UserMainThread;

  return pthread_get(id);
}


EXTERN int pthread_equal(pthread_t t1, pthread_t t2)
{
  return (t1 == t2);
}


static void InitDefault(pthread_t th)
{
  INIT_CONTROL(th);

  memset(&th->specific_data, 0, sizeof(th->specific_data));
  th->specific_data_count = 0;
  th->cleanup = NULL;
  th->joinable = 0; 
  th->detached = th->joinable == PTHREAD_CREATE_DETACHED ? 1 : 0;
  th->priority = 0;
  th->condCBID = INVALID_ID_;
  th->barrierCBID = INVALID_ID_;
  th->joinCBID = INVALID_ID_;

  /* The cancelability state and type of any newly created threads, 
   * including the thread in which main was first invoked, are
   * PTHREAD_CANCEL_ENABLE and PTHREAD_CANCEL_DEFERRED respectively.
   */
  th->cancel_type = PTHREAD_CANCEL_DEFERRED;
  th->cancel_state = PTHREAD_CANCEL_ENABLE;
  th->cancel_pending = 0;
  th->waiting = 0;
  th->terminated = 0;
  th->inWAIT = 0;
  memset(th->priomutex, 0, sizeof(th->priomutex));
}


/*
 * The pthread_create subroutine creates a new thread and 
 * initializes its attributes using the thread attributes object
 * specified by the attr parameter. The new thread inherits its 
 * creating thread's signal mask; but any pending signal of the 
 * creating thread will be cleared for the new thread.
 *
 * The new thread is made runnable, and will start executing the
 * start_routine routine, with the parameter specified by the arg
 * parameter. The arg parameter is a void pointer; it can reference
 * any kind of data. It is not recommended to cast this pointer
 * into a scalar data type (int for example), because the casts 
 * may not be portable.
 *
 * After thread creation, the thread attributes object can be 
 * reused to create another thread, or deleted.
 */

EXTERN int pthread_create(pthread_t *thread, 
                          const pthread_attr_t *attr,
                          void *(*start)(void *), 
                          void *param)
{
  static const char C[16] = "0123456789ABCDEF";
  unsigned int i;
  int res1, res2, result;
  sceGlueParam_t p;
  pthread_t th;
  pthread_attr_t myattr;
  
  PTHREAD_INIT();

  if (attr == NULL)
    pthread_attr_init(&myattr);
  else
    myattr = *attr;

  if (myattr.storage != NULL)
    {
	  th = myattr.storage;
	  th->needsfree = 0;
    }
  else
    {
	  th = PTHREAD_MALLOC(sizeof(struct pthread_storage_t));
	  if (th == NULL)
	    return ENOMEM;
	  th->needsfree = 1;
    }

  InitDefault(th);

  // This code is a little bit kludgy, but we need thread specific data
  // to be returned by a sce* function.  We use the thread name to store
  // such a pointer.
  // See the pthread_get function below for matching code.
  th->name[0] = C[((int)th >> 28) & 0xf];
  th->name[1] = C[((int)th >> 24) & 0xf];
  th->name[2] = C[((int)th >> 20) & 0xf];
  th->name[3] = C[((int)th >> 16) & 0xf];
  th->name[4] = C[((int)th >> 12) & 0xf];
  th->name[5] = C[((int)th >>  8) & 0xf];
  th->name[6] = C[((int)th >>  4) & 0xf];
  th->name[7] = C[((int)th >>  0) & 0xf];
  
  // Update, Feb 2007: we now allow the user to specify a secondary name in the attributes.
  // This name, if set, will appear after the thread pointer in the debugger.
  if (myattr.name[0] == 0) {
      th->name[8] = 0;
  } else {
      th->name[8] = ' ';
      for (i=0; i<sizeof(myattr.name); ++i)
        th->name[9+i] = myattr.name[i];
  }

  myattr.attr |= SCE_KERNEL_THREAD_ATTR_NOTIFY_EXCEPTION;

  SceKernelThreadOptParam* threadOptParam = SCE_NULL;

  SceKernelThreadOptParamForMono threadOptParamForMono = {
   sizeof(SceKernelThreadOptParamForMono), SCE_KERNEL_THREAD_OPT_ATTR_NOTIFY_EXCP_MASK, 0, 0, 0, 0,
   0
  };
  
#if(0)  
  // these values stop all debugging exceptions on native vita
  threadOptParamForMono.notifyExcpMask = SCE_KERNEL_EXCEPTION_TYPE_DABT_PAGE_FAULT | SCE_KERNEL_EXCEPTION_TYPE_PABT_PAGE_FAULT;
#else
  // this value allows debugging exceptions on all native vita created threads
  threadOptParamForMono.notifyExcpMask = 0;
#endif

  threadOptParam = (SceKernelThreadOptParam*)&threadOptParamForMono;

  res1 = sceKernelCreateThread((const char *)th->name, 
                               sceGlue, 
                               myattr.priority, 
                               myattr.stacksize, 
                               myattr.attr,
                               SCE_KERNEL_CPU_MASK_USER_ALL,
                               threadOptParam);

 // printf("sceKernelCreateThread res:0x%x\n",res1);

  if (res1 <= 0) 
    {
      result = EAGAIN;
      goto fail;
    }

  /* Register an event handler */
  /*
  res2 = sceKernelRegisterThreadEventHandler
    ("pthread thread event handler", 
     SCE_KERNEL_TH_SELF,
     SCE_KERNEL_TE_EXIT,
     ThreadEventHandler,
     thread);
  if (res2 <= 0)
    sceCHECK(res2);
  */

  p.start = start;
  p.param = param;
  p.thread = th;

  th->id = res1;
  th->joinable = myattr.joinable;
  th->detached = th->joinable == PTHREAD_CREATE_DETACHED ? 1 : 0;
  th->priority = myattr.priority;

  *thread = th;
  TRACE((void *)th, th->id, "Thread create");

  res2 = sceKernelStartThread(th->id, sizeof(p), &p);

  if (res2 != SCE_OK)
    {
      result = EAGAIN;
      goto fail;
    }
  return res2;

 fail:
  if (th->needsfree)
    PTHREAD_FREE(th);
  return result;
}


/*
 * The pthread_setschedprio() function sets the scheduling priority for 
 * the thread whose thread ID is given by thread to the value given by prio. 
 * If a thread whose policy or priority has been modified by 
 * pthread_setschedprio() is a running thread or is runnable, the effect on 
 * its position in the tread list depends on the direction of the modification 
 * as follows:
 *
 *  If the priority is raised, the thread becomes the tail of the thread list. 
 *  If the priority is unchanged, the thread does not change position in the 
 *  thread list. 
 *  If the priority is lowered, the thread becomes the head of the thread list.
 *
 * Valid priorities are within the range returned by the 
 * sched_get_priority_max() and sched_get_priority_min().
 *
 * If the pthread_setschedprio() function fails, the scheduling priority of 
 * the target thread remains unchanged.
 *
 * The pthread_setschedprio() function might fail if:
 *
 * EINVAL       The value of prio is invalid for the scheduling policy 
 *              of the specified thread. 
 * ENOTSUP      An attempt was made to set the priority to an unsupported 
 *              value. 
 * EPERM        The caller does not have the appropriate permission to set 
 *              the scheduling policy of the specified thread. 
 * EPERM        The implementation does not allow the application to modify
 *              the priority to the value specified. 
 * ESRCH        The value specified by thread does not refer to an existing
 *              thread. 
 */

EXTERN int pthread_setschedprio(pthread_t thread, int prio)
{
  SceUID id;
  //int res;

  if (thread == NULL)
    return EINVAL;

  id = thread->id;

  // Note, I do not know what the kernel does when we change the priority 
  // of a thread.
  //if ((res = sceKernelChangeThreadPriority(id, prio)) == SCE_OK)
  if ((sceKernelChangeThreadPriority(id, prio)) == SCE_OK)
    {
      thread->priority = prio;
      return 0;
    }
/* ######################### TODO : these errors were changed some time, check sys sources
  if (res == (int)SCE_KERNEL_ERROR_UNKNOWN_THID || 
      res == (int)SCE_KERNEL_ERROR_ILLEGAL_THID ||
      res == (int)SCE_KERNEL_ERROR_DORMANT)
    return ESRCH;
  if (res == (int)SCE_KERNEL_ERROR_ILLEGAL_PRIORITY)
    return ENOTSUP;
*/
  return EINVAL;
}


EXTERN int pthread_setschedparam(pthread_t thread, 
                                 int policy, 
                                 const struct sched_param *param)
{
  // Unused parameters
  (void)&policy;
  return pthread_setschedprio(thread, param->sched_priority);
}

/*
 * The pthread_setcancelstate subroutine atomically both sets 
 * the calling thread's cancelability state to the indicated 
 * state and returns the previous cancelability state at the 
 * location referenced by oldstate. Legal values for state are 
 * PTHREAD_CANCEL_ENABLE and PTHREAD_CANCEL_DISABLE.
 *
 * The pthread_setcanceltype subroutine atomically both sets the 
 * calling thread's cancelability type to the indicated type and 
 * returns the previous cancelability type at the location 
 * referenced by oldtype. Legal values for type are 
 * PTHREAD_CANCEL_DEFERRED and PTHREAD_CANCEL_ASYNCHRONOUS.
 * 
 * The cancelability state and type of any newly created threads,
 * including the thread in which main was first invoked, are
 * PTHREAD_CANCEL_ENABLE and PTHREAD_CANCEL_DEFERRED respectively.
 *
 * The pthread_testcancel subroutine creates a cancellation point
 * in the calling thread. The pthread_testcancel subroutine has
 * no effect if cancelability is disabled.
 */

EXTERN int pthread_setcancelstate(int state, int *oldstate)
{
  pthread_t th = pthread_self();
  int old;

  if (state != PTHREAD_CANCEL_ENABLE &&
      state != PTHREAD_CANCEL_DISABLE)
    return EINVAL;

  old = ATOMIC_LW_SW(&th->cancel_state, state);

  if (oldstate != NULL)
    *oldstate = old;

  return 0;
}


EXTERN int pthread_setcanceltype(int type, int *oldtype)
{
  pthread_t th = pthread_self();
  int old;

  if (type != PTHREAD_CANCEL_DEFERRED && 
      type != PTHREAD_CANCEL_ASYNCHRONOUS)
    return EINVAL;

  old = ATOMIC_LW_SW(&th->cancel_type, type);

  if (oldtype != NULL)
    *oldtype = old;

  return 0;
}


EXTERN void pthread_testcancel(void)
{
  int res;
  pthread_t th = pthread_self();

  if (th->cancel_state == PTHREAD_CANCEL_DISABLE || ! th->cancel_pending)
    return;

  // Cancel is enabled and we have a pending cancel request
  // Act immediately!

  cleanup(th);
  if (th->detached)
    {
      detach(th, 1);
    }
  else
    {
      th->id = DELETED_ID_;
      th->returncode = PTHREAD_CANCELED;
    }
  res = sceKernelExitDeleteThread(1);
  sceCHECK(res);
}


/*
 * The pthread_cancel subroutine requests the cancellation of the 
 * thread thread. The action depends on the cancelability of the 
 * target thread:
 *
 *   If its cancelability is disabled, the cancellation request 
 *   is set pending. 
 *
 *   If its cancelability is deferred, the cancellation request 
 *   is set pending till the thread reaches a cancellation point. 
 *
 *   If its cancelability is asynchronous, the cancellation 
 *   request is acted upon immediately; in some cases, it may 
 *   result in unexpected behavior.
 *
 * The cancellation of a thread terminates it safely, using the 
 * same termination procedure as the pthread_exit subroutine.
 */

EXTERN int pthread_cancel(pthread_t thread)
{
  int res;
  int ret = 0;
  int id;
//printf("Cancel - 0\n");

  LOCK_CONTROL(thread);

  if (thread->cancel_state == PTHREAD_CANCEL_DISABLE ||
      thread->cancel_type == PTHREAD_CANCEL_DEFERRED)
    {
//printf("Cancel - 0.1\n");
      thread->cancel_pending = 1;
      if (! thread->inWAIT )
        {
          ret = 0;
          goto exit;
        }
    }
//printf("Cancel - 1\n");

  if (thread->id == DELETED_ID_)
    {
      ret = ESRCH;
      goto exit;
    }
//printf("Cancel - 2\n");

  thread->returncode = PTHREAD_CANCELED;
  id = thread->id;
  thread->id = DELETED_ID_;

  if (pthread_delete_thread_callback)
  {
	  pthread_delete_thread_callback(id);
  }

//  res = sceKernelExitDeleteThread(id);
  res = sceKernelDeleteThread(id);
//printf("Cancel - 3\n");
  sceCHECK(res);

  if (thread->detached) 
    {
//printf("Cancel - 4\n");
      detach(thread, 0);
      return 0; // Semaphore has been cleared by detach
    }

 exit:
  UNLOCK_CONTROL(thread);
//printf("Cancel - 5\n");

  return ret;
}


/*
 * The pthread_exit subroutine terminates the calling thread safely, 
 * and stores a termination status for any thread that may join the 
 * calling thread. The termination status is always a void pointer;
 * it can reference any kind of data. It is not recommended to cast 
 * this pointer into a scalar data type (int for example), because
 * the casts may not be portable. This subroutine never returns.
 *
 * Unlike the exit subroutine, the pthread_exit subroutine does not
 * close files. Thus any file opened and used only by the calling 
 * thread must be closed before calling this subroutine. It is also
 * important to note that the pthread_exit subroutine frees any 
 * thread-specific data, including the thread's stack. Any data
 * allocated on the stack becomes invalid, since the stack is freed
 * and the corresponding memory may be reused by another thread.
 * Therefore, thread synchronization objects (mutexes and condition
 * variables) allocated on a thread's stack must be destroyed before
 * the thread calls the pthread_exit subroutine.
 *
 * Returning from the initial routine of a thread implicitly calls
 * the pthread_exit subroutine, using the return value as parameter.
 *
 * If the thread is not detached, its resources, including the
 * thread ID, the termination status, the thread-specific data,
 * and its storage, are all maintained until the thread is 
 * detached or the process terminates.
 *
 * If another thread joins the calling thread, that thread wakes
 * up immediately, and the calling thread is automatically detached.
 *
 * If the thread is detached, the cleanup routines are popped from
 * their stack and executed. Then the destructor routines from the
 * thread-specific data are executed. Finally, the storage of the
 * thread is reclaimed and its ID is freed for reuse.
 *
 * Terminating the initial thread by calling this subroutine does
 * not terminate the process, it just terminates the initial thread.
 * However, if all the threads in the process are terminated, the
 * process is terminated by implicitly calling the exit subroutine
 * with a return code of 0 if the last thread is detached, or 1
 * otherwise.
 */

EXTERN void pthread_exit(void *status)
{
  pthread_t me = pthread_self();
  
  if (pthread_delete_thread_callback)
  {
	  pthread_delete_thread_callback(sceKernelGetThreadId());
  }

  if (me->detached)
    detach(me, 1);
  else
    me->returncode = status;

  sceKernelExitThread(1);
}


static int CallbackHandler(SceUID notifyId, int count, int arg, void *common)
{
  int res;

  // Unused parameters
  (void)&count;
  (void)&common;
  (void)&notifyId;

  SceUID id = (SceUID)arg;
  res = sceKernelSignalSema(id, 1);
  sceCHECK(res);

  return 0;
}


/*
 * The pthread_join subroutine blocks the calling thread until 
 * the thread thread terminates. The target thread's termination 
 * status is returned in the status parameter.
 *
 * If the target thread is already terminated, but not yet 
 * detached, the subroutine returns immediately. It is impossible
 * to join a detached thread, even if it is not yet terminated. 
 *
 * This subroutine does not itself cause a thread to be 
 * terminated. It acts like the pthread_cond_wait subroutine 
 * to wait for a special condition.
 */

EXTERN int pthread_join(pthread_t thread, void **value_ptr)
{
  int res, resCB, ret;

  if (thread == NULL || thread->joinable != PTHREAD_CREATE_JOINABLE)
    return EINVAL;

  LOCK_CONTROL(thread);

  if (thread->id == DELETED_ID_)
    {
      if (value_ptr != NULL) 
        *value_ptr = thread->returncode;
      UNLOCK_CONTROL(thread);
      return 0;
    }

  thread->waiting++;

  if (thread->joinCBID == INVALID_ID_)
    {
      res = sceKernelCreateCallback("Wait Thread End", 0, CallbackHandler, NULL);
      if (res <= 0)
        {
          sceCHECK(res);
          UNLOCK_CONTROL(thread);
          return res;
        }
      thread->joinCBID = res;
    }

  res = sceKernelNotifyCallback(thread->joinCBID, (int)thread->control);
  sceCHECK(res);

  res = sceKernelWaitThreadEndCB(thread->id, NULL, NULL);
  if ((resCB = sceKernelCheckCallback()) == 0)
    {
      /*
        The side effect of the call to CheckCallback
        is to invoke the callback!
      */
    }
  else if (resCB == 1)
    {
    }
  else
    sceCHECK(resCB);

  LOCK_CONTROL(thread);

  thread->waiting--;

  if (value_ptr != NULL) 
    *value_ptr = thread->returncode;
  
  if (thread->waiting == 0)
    {
      /*
       * The target thread is automatically detached after all joined 
       * threads have been woken up.
       */
      detach(thread, 0);
      return 0;     // Semaphore has been cleared by detach
    }
  ret = 0;
  /*
    else if (res == SCE_KERNEL_ERROR_ILLEGAL_THID ||
           res == SCE_KERNEL_ERROR_CAN_NOT_WAIT || 
           res == SCE_KERNEL_ERROR_RELEASE_WAIT ||
           res == SCE_KERNEL_ERROR_WAIT_TIMEOUT ||
           res == SCE_KERNEL_ERROR_DORMANT)
    ret = EDEADLK;
  else if (res == SCE_KERNEL_ERROR_UNKNOWN_THID)
    ret = ESRCH;
  else
    // elsif (res == SCE_KERNEL_ERROR_ILLEGAL_CONTEXT)
    // elsif (res == SCE_KERNEL_ERROR_ILLEGAL_ARGUMENT)
    ret = EINVAL;

  res = sceKernelSignalSema(thread->control, 1);
  sceCHECK(res);
  */

  UNLOCK_CONTROL(thread);
  return ret;
}


/*
 * The pthread_detach subroutine is used to indicate to the 
 * implementation that storage for the thread whose thread ID 
 * is in the location thread can be reclaimed when that thread 
 * terminates. This storage shall be reclaimed on process exit, 
 * regardless of whether the thread has been detached or not, 
 * and may include storage for thread return value. If thread 
 * has not yet terminated, pthread_detach shall not cause it 
 * to terminate. Multiple pthread_detach calls on the same 
 * target thread causes an error.
 */

EXTERN int pthread_detach(pthread_t thread)
{
  if (thread == NULL || 
      thread->joinable != PTHREAD_CREATE_JOINABLE)
    return EINVAL;

  if (test_and_set_bit(0, &thread->detached))
    return EINVAL;

  return 0;
}


EXTERN int pthread_getschedparam(pthread_t thread, int *policy, 
                                 struct sched_param *param)
{
  *policy = 0;	// Unused.
  param->sched_priority = thread->priority;
  return 0;
}


EXTERN int pthread_attr_init(pthread_attr_t *attr)
{
  attr->joinable = PTHREAD_CREATE_JOINABLE;
  attr->stacksize = PTHREAD_STACK_MIN;
//  attr->priority = SCE_KERNEL_PROCESS_PRIORITY_USER_LOW;
  attr->priority = SCE_KERNEL_PROCESS_PRIORITY_USER_LOW + 1; // behavior changed from SDK 0990
  attr->attr = 0;
  attr->name[0] = 0;
  attr->storage = 0;
  return 0;
}


EXTERN int pthread_attr_destroy(pthread_attr_t *attr)
{
  (void)&attr;
  return 0;
}


/*
 * The useVFPU functions are used to indicate that the VFPU is used
 */
/*
EXTERN int pthread_attr_setuseVFPU_np(pthread_attr_t *attr, char vfpu)
{
  if (vfpu)
    attr->attr |= SCE_KERNEL_TH_USE_VFPU;
  else
    attr->attr &= ~SCE_KERNEL_TH_USE_VFPU;

  return 0;
}
*/

/*
EXTERN int pthread_attr_getuseVFPU_np(const pthread_attr_t *attr, char *vfpu)
{
  *vfpu = attr->attr & SCE_KERNEL_TH_USE_VFPU ? 1 : 0;
  return 0;
}
*/

/*
 * The storage attribute specifies the storage for the pthread_t, to avoid an allocation.
 */

EXTERN int pthread_attr_setstorage_np(pthread_attr_t *attr, pthread_storage_t *storage)
{
	attr->storage = storage;
	return 0;
}


EXTERN int pthread_attr_getstorage_np(pthread_attr_t *attr, pthread_storage_t **storage)
{
	*storage = attr->storage;
	return 0;
}


/*
 * The name attribute specifies a name to be used in the debugger.
 * For setname, the name will be truncated at 22 bytes.
 * For getname, the buffer must be at least 23 bytes in size.
 */

EXTERN int pthread_attr_setname_np(pthread_attr_t *attr, const char *name)
{
  unsigned int i;

  if (attr == NULL || name == NULL)
    return EINVAL;
  
  for (i=0; i<sizeof(attr->name)-1; ++i) {
    if ((attr->name[i] = name[i]) == 0)
	  break;
  }
  attr->name[sizeof(attr->name)-1] = 0;
  return 0;
}


EXTERN int pthread_attr_getname_np(const pthread_attr_t *attr, char *name)
{
  unsigned int i;

  if (attr == NULL || name == NULL)
    return EINVAL;
  
  for (i=0; i<sizeof(attr->name)-1; ++i) {
    if ((name[i] = attr->name[i]) == 0)
	  break;
  }
  name[sizeof(attr->name)-1] = 0;
  return 0;
}


EXTERN int pthread_attr_setdetachstate(pthread_attr_t *attr, int state)
{
  if (state != PTHREAD_CREATE_DETACHED && state != PTHREAD_CREATE_JOINABLE)
    return EINVAL;

  attr->joinable = state;
  return 0;
}


EXTERN int pthread_attr_getdetachstate(const pthread_attr_t *attr, int *state)
{
  *state = attr->joinable;
  return 0;
}


EXTERN int pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize)
{
  if (stacksize < PTHREAD_STACK_MIN)
    return EINVAL;

  attr->stacksize = stacksize;
  return 0;
}


EXTERN int pthread_attr_getstacksize(const pthread_attr_t *attr, size_t *stacksize)
{
  *stacksize = attr->stacksize;
  return 0;
}


EXTERN int pthread_attr_getschedparam(const pthread_attr_t *attr, struct sched_param *param)
{
  param->sched_priority = attr->priority;
  return 0;
}


EXTERN int pthread_attr_setschedparam(pthread_attr_t *attr, const struct sched_param *param)
{
  attr->priority = param->sched_priority;
  return 0;
}


pthread_mutex_t TRACE_MUTEX;
pthread_mutex_t _PTGLOBAL_;

EXTERN void pthread_init_()
{
  pthread_mutexattr_t attr;

  EXECUTE_ONCE_BEGIN();
    {
      pthread_mutexattr_init(&attr);
      pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
      pthread_mutex_init(&_PTGLOBAL_, &attr);
      
      pthread_mutex_init(&TRACE_MUTEX, NULL);
      
      // Create a pthread data structure for the UserMain thread
      UserMainThread = &UserMainThreadStorage;
      UserMainThread->id = sceKernelGetThreadId();
      InitDefault(UserMainThread);
      UserMainThread->joinable = 0; 
      UserMainThread->detached = 1;
      //  UserMainThread->priority = attr->priority;
      
      pthread_enter_critical = enter_critical_func;
      pthread_leave_critical = leave_critical_func;
    }
  EXECUTE_ONCE_END();
}


EXTERN pthread_t pthread_get(SceUID uid)
{
  SceKernelThreadInfo info;
  int p = 0, i;
  char c = 0;
#define x -1
  static const char nybble[256] = {
	  x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x,
	  x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, 0,1,2,3, 4,5,6,7, 8,9,x,x, x,x,x,x,
	  x,10,11,12, 13,14,15,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x,
	  x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x,
	  x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x,
	  x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x,
	  x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x,
	  x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x,
  };
#undef x

  info.size = sizeof(SceKernelThreadInfo);
  sceKernelGetThreadInfo(uid, &info);
  
  for (i=0; i<8 && c>=0; ++i)
    {
      c = nybble[(unsigned char)info.name[i]];
      p = (p << 4) | c;
    }

  if (c < 0)
    {
      // Not a pthread! Print a helpful message and crash into the debugger.
      pthread_printf_np("Error: pthread API call made from non-pthread thread\n");
    }

  return (pthread_t)p;
}


EXTERN int ERROR_errno_sce(int res)
{
  switch (res)
  {
    case SCE_OK:           res = 0;         break;
	case SCE_KERNEL_ERROR_WAIT_TIMEOUT: res = ETIMEDOUT; break;
	case SCE_KERNEL_ERROR_NO_MEMORY:    res = ENOMEM;    break;
	case SCE_KERNEL_ERROR_ILLEGAL_ATTR: res = EPERM;     break;
	default: res = EINVAL;
  }
  return res;
}



EXTERN void PCHECK_(const char *file, int line, int rc)
{
  pthread_printf_np("Error: %s(%i) errno: %i\n", file, line, rc);
}


EXTERN void SCECHECK_(const char *file, int line, int rc)
{
  pthread_printf_np("Error: %s(%i) sce error: %08x\n", file, line, rc);
}

EXTERN int pthread_vprintf_np(const char *fmt, va_list ap)
{
    // Turns out that printf/vprintf/vfprintf are not threadsafe as of PSP SDK 2.6.
    // However, fwrite and fflush are. So we'll do it the old-fashioned way.
    char buf[256]; // impose a maximum string length
    int len = vsnprintf(buf,sizeof(buf),fmt,ap);
    if (len > (int)sizeof(buf))
		len = (int)sizeof(buf);
    fwrite(buf,len,1,stdout);
    fflush(stdout);
	return len;
}

EXTERN int pthread_printf_np(const char *fmt, ...)
{
	va_list ap;
	va_start(ap,fmt);
	int res = pthread_vprintf_np(fmt,ap);
	va_end(ap);
	return res;
}

EXTERN SceUInt getDeltaTime(const struct timespec *abstime)
{
  timespec now;
  int delta;
  long long a, n;

  pthread_getsystemtime_np(&now);

  a = timespec2usec(abstime);
  n = timespec2usec(&now);
  delta = a - n;
  if (delta < 0)
    delta = 0;
  return delta;
}
