//Sony Computer Entertainment Confidential
#ifndef _H_pthread_impl_psp
#define _H_pthread_impl_psp

#include <stdio.h>

// SceUID values
#define INVALID_ID_          ((SceUID)(-1))
#define STATIC_INIT_ID_      ((SceUID)(-2))
#define DELETED_ID_          ((SceUID)(-3))

// Signatures used by static initializers
#define RWLOCK_SIG_           1
#define MUTEX_SIG_            2
#define COND_SIG_             3

// C++ operators to make pthread types a little more compatible with
//  code that expects pointers instead of structs
#define PTHREAD_PSP_TYPES_ARE_STRUCTS 1
#ifdef __cplusplus
#define PTHREAD_CPP_OPERATORS(T,id) \
	inline T() {} \
	inline T(int rhs)                  { *this = rhs; } \
	inline T(void *rhs)                { *this = rhs; } \
	inline T& operator= (int rhs)      { if (rhs==0) id=INVALID_ID_; return *this; } \
	inline T& operator= (void *rhs)    { if (rhs==0) id=INVALID_ID_; return *this; } \
	inline bool operator== (int rhs)   { return (rhs==0) && (id==INVALID_ID_); } \
	inline bool operator== (void *rhs) { return (rhs==0) && (id==INVALID_ID_); } \
	inline bool operator!= (int rhs)   { return !(*this == rhs); } \
	inline bool operator!= (void *rhs) { return !(*this == rhs); } \
	inline operator bool()             { return (*this != 0); }
#else
#define PTHREAD_CPP_OPERATORS(T,id)
#endif


// check macro
EXTERN void PCHECK_(const char *file, int line, int rc);
#define pCHECK(rc)      do { if (rc) PCHECK_(__FILE__, __LINE__, rc); } while(0)


// Number of per-thread keys that we support
#define PTHREAD_KEYS_MAX_     48


/* Declaration of a control to be put inside a pthread struct   */
#define CONTROL   SceUID control

typedef struct pthread_cleanup_t
{
  void                      (*func)(void *);
  void                      *arg;
  struct pthread_cleanup_t  *next;
} pthread_cleanup_t;


typedef struct pthread_storage_t
{
  CONTROL;                              // Lock control
  SceUID                id;             // sceID of the thread
  SceUID                condCBID;       // Callback for support of cond variables
  SceUID                barrierCBID;    // Callback for support of barriers
  SceUID                joinCBID;       // Callback for support of join
  pthread_cleanup_t    *cleanup;
  volatile long         cancel_state;
  volatile long         cancel_type;
  int                   waiting;        // Number of threads waiting to join this thread
  unsigned long         detached;
  void                 *returncode;     // Thread return code
  char                  name[SCE_UID_NAMELEN+1];
  char                  cancel_pending;
  char                  joinable;
  char                  inWAIT;         // Indicates the thread is waiting for a pthread mutex or cond
  char                  terminated;     // Thread is terminated
  char                  needsfree;      // Thread storage belongs to pthread lib and will be freed with PTHREAD_FREE

  /* Original priority of the thread.  The thread priority can be temporarily
     changed when waiting on mutex.
  */
  unsigned char priority;

  /* 
     The priomutex array indicates how many mutex of each priority are held 
     by this thread.
     We assume there will be no more than 128 mutex per priority
     This is needed to implement priority ceiling.
  */
  unsigned char priomutex[128];
  
  /* Specific data has been moved inline instead of being dynamically allocated when accessed */
  int                   specific_data_count;
  const void           *specific_data[PTHREAD_KEYS_MAX_];
  
#ifdef __cplusplus
  /* Helper operators for C++ */
  PTHREAD_CPP_OPERATORS(pthread_storage_t, id)
  inline operator struct pthread_storage_t * () { return this; }
  inline operator SceUID () { return id; }
#endif
} pthread_storage_t;
typedef pthread_storage_t* pthread_t;

typedef struct pthread_attr_t
{
  int joinable;
  size_t stacksize;
  int priority;
  SceUInt attr;
  char name[SCE_UID_NAMELEN - 9 + 1];
  pthread_storage_t *storage;
} pthread_attr_t;


typedef struct pthread_barrierattr_t
{
} pthread_barrierattr_t;


typedef struct pthread_barrier_t
{
  CONTROL;
  SceUID        queue;
  long          neededcount;
  long          count;

  PTHREAD_CPP_OPERATORS(pthread_barrier_t,control)
} pthread_barrier_t;


typedef struct pthread_mutexattr_t
{
  int MaxCount;
  int scheduling;               // PTHREAD_QUEUE_*
  int prioceiling;              // Priority ceiling
  char protocol;                // PTHREAD_PRIO_*
  char type;
} pthread_mutexattr_t;


typedef struct pthread_key_t
{
  int key;
} pthread_key_t;


typedef struct pthread_mutex_t
{
  SceUID        id;
  pthread_t     owner;          // The pthread owning the lock
  int           recursivecount; // Recursive mutex only
  int           prioceiling;    // Priority ceiling
  char          type;
  char          protocol;       // PTHREAD_PRIO_*
  /* 
     The priowait array indicates the priority of the threads
     waiting for this mutex
     We assume there will be no more than 128 mutex per priority
     This is needed to implement priority ceiling.
  */
  unsigned char priowait[128];

  PTHREAD_CPP_OPERATORS(pthread_mutex_t,id)
} pthread_mutex_t;

#define PTHREAD_MUTEX_INITIALIZER_              { STATIC_INIT_ID_, (pthread_t)MUTEX_SIG_, 0, 0, (char)PTHREAD_MUTEX_NORMAL,     0, {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0} }
#define PTHREAD_RECURSIVE_MUTEX_INITIALIZER_    { STATIC_INIT_ID_, (pthread_t)MUTEX_SIG_, 0, 0, (char)PTHREAD_MUTEX_RECURSIVE,  0, {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0} }
#define PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_   { STATIC_INIT_ID_, (pthread_t)MUTEX_SIG_, 0, 0, (char)PTHREAD_MUTEX_ERRORCHECK, 0, {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0} }


typedef struct pthread_cond_t
{
  CONTROL;
  int             NbWait;       // Number of threads waiting
  SceUID          lock;         // Sema used to queue the threads

  PTHREAD_CPP_OPERATORS(pthread_cond_t,control)
} pthread_cond_t;

#define PTHREAD_COND_INITIALIZER_               { STATIC_INIT_ID_, (int)COND_SIG_, 0 }

typedef struct pthread_condattr_t
{
} pthread_condattr_t;


typedef struct pthread_rwlock_t
{
  SceUID        rdwrQ;          // Readers and writers queue
  int           writelock;      // A writer has the control

  PTHREAD_CPP_OPERATORS(pthread_rwlock_t,rdwrQ)
} pthread_rwlock_t;

#define PTHREAD_RWLOCK_INITIALIZER_             { STATIC_INIT_ID_, (int)RWLOCK_SIG_ }

typedef struct pthread_rwlockattr_t
{
} pthread_rwlockattr_t;


typedef struct pthread_spinlock_t
{
  unsigned long lock;
} pthread_spinlock_t;

#define PTHREAD_SPINLOCK_INITIALIZER_           { 0 }

typedef struct pthread_mbx_t
{
  SceUID id;
} pthread_mbx_t;


typedef struct pthread_mbxattr_t
{
  int qwait;
  int qmsg;
} pthread_mbxattr_t;


typedef struct pthread_msgpipe_t
{
  SceUID id;

  PTHREAD_CPP_OPERATORS(pthread_msgpipe_t,id)
} pthread_msgpipe_t;


typedef struct pthread_msgpipeattr_t
{
  int attr;
  int bufsize;
  int partition;
} pthread_msgpipeattr_t;


typedef struct pthread_eventflag_t
{
  SceUID id;

  PTHREAD_CPP_OPERATORS(pthread_eventflag_t,id)
} pthread_eventflag_t;


typedef struct pthread_eventflagattr_t
{
  int attr;
  int init;
} pthread_eventflagattr_t;


typedef struct pthread_once_t 
{
  int done;
  pthread_mutex_t in_progress;
} pthread_once_t;

#define PTHREAD_ONCE_INIT_                      { 0, PTHREAD_MUTEX_INITIALIZER }

#define PTHREAD_INIT()          do { pthread_init_(); } while(0)
#define CHECK_PT_PTR(ptr)       do { if (ptr == NULL) return EINVAL; } while(0)

/* -----------------------------
     malloc/free
   ----------------------------- */
# define PTHREAD_MALLOC(size)       malloc(size) 
# define PTHREAD_FREE(ptr)          do { free((void*)(ptr)); (ptr) = NULL; } while(0)

#define CLEAR(x)                do { memset(x, 0, sizeof(*x)); } while(0)

EXTERN void(* pthread_enter_critical)(void);
EXTERN void(* pthread_leave_critical)(void);


/**
 * @{
 */

/**
 * Note on critical sections:
 *
 * The {ENTER | LEAVE}_CRITICAL is used to globaly lock the pthread
 * package.  Such a lock should be used only when absolutely necessary.
 * The main cases are:
 *
 * - pthread initialization
 * - access to the global heap
 * - initialization of STATIC_INIT objects
 *
 * In any other case the *_CONTROL mechanism should be used instead.
 * LOCK_CONTROL will only lock a single object as opposed to the entire
 * package.  This does not make any difference on a uni processor machine,
 * but will have a performance impact on a multi processor system.
 */

/* Structure used to Lock/Unlock a pthread struct      */

#define LOCK_CONTROL(c)                                 \
do {                                                    \
  int res_ = sceKernelWaitSema((c)->control, 1, NULL);  \
  sceCHECK(res_);                                       \
} while(0)

#define UNLOCK_CONTROL(c)                               \
do {                                                    \
  int res_ = sceKernelSignalSema((c)->control, 1);      \
  sceCHECK(res_);                                       \
} while(0)

#define INIT_CONTROL(c)                                 \
do {                                                    \
  int res_ = sceKernelCreateSema("pthread control",     \
                                SCE_KERNEL_ATTR_TH_FIFO,   \
                                1, 1, NULL);            \
  if (res_ > 0)                                         \
    c->control = res_;                                  \
  else                                                  \
    sceCHECK(res_);                                     \
} while(0)

#define DELETE_CONTROL(c)                               \
do {                                                    \
  int res_ = sceKernelDeleteSema((c)->control);         \
  sceCHECK(res_);                                       \
} while(0)


#define ENTER_CRITICAL()        do { pthread_enter_critical(); } while(0)
#define LEAVE_CRITICAL()        do { pthread_leave_critical(); } while(0)

/**
 * @}
 */



EXTERN void pthread_cleanupspecific(pthread_t me);


EXTERN pthread_t pthread_get(SceUID id);

/**
 * The EXECUTE_ONCE_* mechansim is different from the ONCE_INIT 
 * mechanism in pthread.  First it can be used before pthread is
 * initialized.  It also uses a busy loop to wait until the 
 * execution is done.
 *
 * control variable:
 * 0: not done, 
 * 1: in progress, 
 * 2: done
 */
#define EXECUTE_ONCE_BEGIN()            	\
static volatile long control = 0;        	\
{                                       	\
  int s;                                	\
  if (control == 2)                     	\
    goto _execute_once_done_;           	\
                                        	\
  if ((s = ATOMIC_LW_SW(&control, 1)) == 2) {   \
      control = 2;                      	\
      goto _execute_once_done_;            	\
  }                                             \
  else if (s == 1) {                     	\
      while (s != 2)                    	\
        sceKernelDelayThread(1);        	\
      goto _execute_once_done_;            	\
  }                                             \
}

#define EXECUTE_ONCE_END()              	\
  control = 2;                          	\
  _execute_once_done_: ;                	\


/*
 * Transfrom an SCE error code into an errno error code
 */

EXTERN int ERROR_errno_sce(int res);


/*
 * Debug support 
 */

EXTERN void SCECHECK_(const char *file, int line, int rc);
#define sceCHECK(rc)    do { if (rc != SCE_OK) SCECHECK_(__FILE__, __LINE__, rc); } while(0)

extern pthread_mutex_t _PTGLOBAL_;
EXTERN int pthread_printf_np(const char *fmt, ...);

#if 0 && (defined(DEBUG) && DEBUG)
extern pthread_mutex_t TRACE_MUTEX;
static inline void TRACE(void *p, SceUID s, const char *msg)
{
  if (p == &TRACE_MUTEX)
    return;

  pthread_mutex_lock(&TRACE_MUTEX);
  if (p != NULL)
    pthread_printf_np("%08x ", p);
  else
    pthread_printf_np("         ");

  if (s != 0)
    pthread_printf_np("%08x ", s);
  else
    pthread_printf_np("         ");

  pthread_printf_np("%s\n", msg);
  pthread_mutex_unlock(&TRACE_MUTEX);
}
#else
#undef TRACE
#define TRACE(p, s, msg)
#endif


EXTERN void pthread_init_();
EXTERN void pthread_cleanupspecific_(pthread_t me);

/*
 * timespec should be defined in <time.h> but appears to be 
 * missing from the SN headers.
 *
 */

struct timespec
{
  int tv_sec;
  int tv_nsec;
};

static inline SceUInt timespec2usec(const struct timespec *t)
{
  return (t->tv_sec * 1000 * 1000) + ((t->tv_nsec +500) / 1000);
};

EXTERN SceUInt getDeltaTime(const struct timespec *abstime);

#endif /* _H_pthread_impl_psp */
