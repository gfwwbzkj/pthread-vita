//Sony Computer Entertainment Confidential
/*
 * This is an implementation of pthread for the PSP.  An implementation
 * of pthread for Windows is available at: 
 *      http://sources.redhat.com/pthreads-win32/
 *
 * All the applicable tests coming with the win32 implementation pass
 * with this implementation on the PSP.
 *
 * The pthread specification for the PSP has been extended to support 
 * PSP specific features (see names ending with _NP or _np)
 *
 * Contents:
 *      PThreads Limits
 *      Static initializer 
 *      Type declaration 
 *      Threads 
 *      Thread specific data 
 *      Barriers 
 *      Condition Variables 
 *      Mutexes 
 *      Read/Write Locks 
 *      RealTime Scheduling 
 *      Spin Locks 
 *      Mailbox support (_np) 
 *      Event flags (_np) 
 *      Message pipe (_np)
 *      Other Non-Portable functions (_np) 
 *      Unimplemented functions 
 *      Additional functions that are not part of pthread but 
 *       are available on POSIX systems                                
 *      Private part 
 *
 */
#ifndef _H_pthread_psp
#define _H_pthread_psp

#include <kernel.h>
#include <stdlib.h>
#include <errno.h>

#include "sched.h"
#include "pthread_atomic.h"
#include "pthread_impl.h"


#define _THREAD_SAFE 

/* Most compiler do not support the restrict keyword */
#define restrict __restrict


/*
 * POSIX 1003.1c-1995 options / 1003.1 2001 options
 */

#define _POSIX_THREADS                          2001L   // We can use threads
#define _POSIX_THREAD_ATTR_STACKSIZE            2001L   // Stack size is supported
#define _POSIX_THREAD_PRIO_INHERIT              2001L   // Priority inheritance mutexes supported
#define _POSIX_THREAD_PRIO_PROTECT              2001L   // Priority ceiling mutexes supported
#define _POSIX_READER_WRITER_LOCKS              2001L   // Multiple readers / single writer
#define _POSIX_SPIN_LOCKS                       2001L   // Spin lock
#define _POSIX_BARRIERS                         2001L   // Barriers
#define _POSIX_THREAD_PRIORITY_SCHEDULING       2001L   // Realtime scheduling is supported

/* 
 * The following features are not supported
 */

#define _POSIX_THREAD_ATTR_STACKADDR            -1      // Stack addr is supported
#define _POSIX_SEMAPHORES                       -1      // Old style semaphores
#define _POSIX_THREAD_PROCESS_SHARED            -1      // Mutexes and conditions can be shared across processes
#define _POSIX_THREAD_SAFE_FUNCTIONS            -1      // "_r" functions are supported


/* ************************************* */
/* ********** PThreads Limits ********** */
/* ************************************* */
 
/** @defgroup Limits PThreads Limits
 *
 * @{
 */

/*
 * POSIX 1003.1c-1995 limits
 */

/**
 * Maximum number of attempts to destroy a thread's thread-specific data (>= 4)
 */
#define PTHREAD_DESTRUCTOR_ITERATIONS   4       
                                                

/**
 * Maximum number of thread-specific data keys per process (>= 128), but we slim that down below spec
 */
#define PTHREAD_KEYS_MAX                PTHREAD_KEYS_MAX_


/**
 * Minimum stack size 
 * Note: It seems that 4K is the minimum stack required to run any Boost tests
 */
#define PTHREAD_STACK_MIN               (4*1024)


/**
 * Maximum number of threads per process (>= 64)
 */
#define PTHREAD_THREADS_MAX             128     

/** @} */


#define _POSIX_THREAD_THREADS_MAX               PTHREAD_THREADS_MAX
#define _POSIX_THREAD_KEYS_MAX                  PTHREAD_KEYS_MAX
#define _POSIX_THREAD_DESTRUCTOR_ITERATIONS     PTHREAD_DESTRUCTOR_ITERATIONS


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


typedef void (*pthread_callback_t)(SceUID threadId);


/** 
 * Make sure we do not use a value that would be interpreted as an 
 * errno number
 */
#define PTHREAD_BARRIER_SERIAL_THREAD   (-2)


/** 
 * Just need a unique address 
 */
#define PTHREAD_CANCELED                ((void *) pthread_cancel)


/* **************************************** */
/* ********** Static Initializer ********** */
/* **************************************** */
 
/** @defgroup Static Static Initializers
 *
 * @{
 */

/**
 * The PTHREAD_ONCE_INIT macro initializes the static once 
 * synchronization control structure once_block, used for 
 * one-time initializations with the pthread_once 
 * (pthread_once Subroutine) subroutine. The once synchronization 
 * control structure must be static to ensure the unicity of the 
 * initialization.
 */
#define PTHREAD_ONCE_INIT                       PTHREAD_ONCE_INIT_

#define PTHREAD_MUTEX_INITIALIZER               PTHREAD_MUTEX_INITIALIZER_
#define PTHREAD_COND_INITIALIZER                PTHREAD_COND_INITIALIZER_
#define PTHREAD_RWLOCK_INITIALIZER              PTHREAD_RWLOCK_INITIALIZER_
#define PTHREAD_RECURSIVE_MUTEX_INITIALIZER     PTHREAD_RECURSIVE_MUTEX_INITIALIZER_
#define PTHREAD_SPINLOCK_INITIALIZER            PTHREAD_SPINLOCK_INITIALIZER_
#define PTHREAD_ERRORCHECK_MUTEX_INITIALIZER    PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_

/** @} */


/* ************************************** */
/* ********** Type declaration ********** */
/* ************************************** */

typedef struct timespec timespec;

/* ***************************** */
/* ********** Threads ********** */
/* ***************************** */

/** @defgroup Threads Threads
 *
 * @{
 */

enum {
  PTHREAD_PROCESS_PRIVATE,      // No support for shared
};


EXTERN int pthread_attr_init(pthread_attr_t *attr);
EXTERN int pthread_attr_destroy(pthread_attr_t *attr);


enum {
  PTHREAD_CREATE_DETACHED,
  PTHREAD_CREATE_JOINABLE,
};

/** 
 * The detachstate attribute controls whether the thread is created in 
 * a detached state. If the thread is created detached, then use of the 
 * ID of the newly created thread by the pthread_detach() or 
 * pthread_join() function is an error.
 *
 * The pthread_attr_getdetachstate() and pthread_attr_setdetachstate() 
 * functions, respectively, shall get and set the detachstate attribute 
 * in the attr object.
 *
 * For pthread_attr_getdetachstate(), detachstate shall be set to
 * either PTHREAD_CREATE_DETACHED or PTHREAD_CREATE_JOINABLE.
 *
 * For pthread_attr_setdetachstate(), the application shall set detachstate 
 * to either PTHREAD_CREATE_DETACHED or PTHREAD_CREATE_JOINABLE.
 *
 * A value of PTHREAD_CREATE_DETACHED shall cause all threads created 
 * with attr to be in the detached state, whereas using a value of
 * PTHREAD_CREATE_JOINABLE shall cause all threads created with attr to
 * be in the joinable state. The default value of the detachstate 
 * attribute shall be PTHREAD_CREATE_JOINABLE.
 */

EXTERN int pthread_attr_setdetachstate(pthread_attr_t *attr, int state);


/** 
 * The detachstate attribute controls whether the thread is created in 
 * a detached state. If the thread is created detached, then use of the 
 * ID of the newly created thread by the pthread_detach() or 
 * pthread_join() function is an error.
 *
 * The pthread_attr_getdetachstate() and pthread_attr_setdetachstate() 
 * functions, respectively, shall get and set the detachstate attribute 
 * in the attr object.
 *
 * For pthread_attr_getdetachstate(), detachstate shall be set to
 * either PTHREAD_CREATE_DETACHED or PTHREAD_CREATE_JOINABLE.
 *
 * For pthread_attr_setdetachstate(), the application shall set detachstate 
 * to either PTHREAD_CREATE_DETACHED or PTHREAD_CREATE_JOINABLE.
 *
 * A value of PTHREAD_CREATE_DETACHED shall cause all threads created 
 * with attr to be in the detached state, whereas using a value of
 * PTHREAD_CREATE_JOINABLE shall cause all threads created with attr to
 * be in the joinable state. The default value of the detachstate 
 * attribute shall be PTHREAD_CREATE_JOINABLE.
 */

EXTERN int pthread_attr_getdetachstate(const pthread_attr_t *attr, int *state);


/*
 * The useVFPU functions are used to indicate that the VFPU is used
 */

/*
EXTERN int pthread_attr_setuseVFPU_np(pthread_attr_t *attr, char vfpu);
EXTERN int pthread_attr_getuseVFPU_np(const pthread_attr_t *attr, char *vfpu);
*/

/*
 * The name attribute specifies a name to be used in the debugger.
 * For setname, names longer than 22 characters will be truncated.
 * For getname, the output buffer must be at least 23 bytes in size.
 */
EXTERN int pthread_attr_setname_np(pthread_attr_t *attr, const char *name);
EXTERN int pthread_attr_getname_np(const pthread_attr_t *attr, char *name);


/**
 * The pthread_getschedparam() and pthread_setschedparam() 
 * allow the scheduling policy and scheduling parameters of 
 * individual threads within a multi-threaded process to be
 * retrieved and set. For SCHED_FIFO and SCHED_RR, the only 
 * required member of the sched_param structure is the 
 * priority sched_priority. For SCHED_OTHER, the affected 
 * scheduling parameters are implementation-dependent.
 * 
 * The pthread_getschedparam() function retrieves the scheduling 
 * policy and scheduling parameters for the thread whose
 * thread ID is given by thread and stores those values in 
 * policy and param, respectively. The priority value returned
 * from pthread_getschedparam() shall be the value specified by 
 * the most recent pthread_setschedparam() or pthread_create()
 * call affecting the target thread. It shall not reflect any 
 * temporary adjustments to its priority as a result of any 
 * priority inheritance or ceiling functions. The
 * pthread_setschedparam() function sets the scheduling policy
 * and associated scheduling parameters for the thread whose 
 * thread ID is given by thread to the policy and associated 
 * parameters provided in policy and param, respectively. 
 *
 * The policy parameter may have the value SCHED_OTHER, that
 * has implementation-dependent scheduling parameters, SCHED_FIFO
 * or SCHED_RR, that have the single scheduling parameter, priority. 
 *
 * If the pthread_setschedparam() function fails, no scheduling 
 * parameters will be changed for the target thread. 
 *
 * If successful, the pthread_getschedparam() and 
 * pthread_setschedparam() functions return zero. Otherwise, an
 * error number is returned to indicate the error. 
 * 
 * The pthread_getschedparam() and pthread_setschedparam() functions
 * will fail if: 
 * 
 * ENOSYS       The option _POSIX_THREAD_PRIORITY_SCHEDULING 
 *              is not defined and the implementation does not
 *              support the function. 
 *
 * The pthread_getschedparam() function may fail if: 
 * 
 * ESRCH        The value specified by thread does not refer to
 *              a existing thread. 
 */

EXTERN int pthread_getschedparam(pthread_t thread,
                                 int *restrict policy,
                                 struct sched_param *restrict param);

/**
 * The pthread_getschedparam() and pthread_setschedparam() 
 * allow the scheduling policy and scheduling parameters of 
 * individual threads within a multi-threaded process to be
 * retrieved and set. For SCHED_FIFO and SCHED_RR, the only 
 * required member of the sched_param structure is the 
 * priority sched_priority. For SCHED_OTHER, the affected 
 * scheduling parameters are implementation-dependent.
 * 
 * The pthread_getschedparam() function retrieves the scheduling 
 * policy and scheduling parameters for the thread whose
 * thread ID is given by thread and stores those values in 
 * policy and param, respectively. The priority value returned
 * from pthread_getschedparam() shall be the value specified by 
 * the most recent pthread_setschedparam() or pthread_create()
 * call affecting the target thread. It shall not reflect any 
 * temporary adjustments to its priority as a result of any 
 * priority inheritance or ceiling functions. The
 * pthread_setschedparam() function sets the scheduling policy
 * and associated scheduling parameters for the thread whose 
 * thread ID is given by thread to the policy and associated 
 * parameters provided in policy and param, respectively. 
 *
 * The policy parameter may have the value SCHED_OTHER, that
 * has implementation-dependent scheduling parameters, SCHED_FIFO
 * or SCHED_RR, that have the single scheduling parameter, priority. 
 *
 * If the pthread_setschedparam() function fails, no scheduling 
 * parameters will be changed for the target thread. 
 *
 * If successful, the pthread_getschedparam() and 
 * pthread_setschedparam() functions return zero. Otherwise, an
 * error number is returned to indicate the error. 
 * 
 * The pthread_getschedparam() function may fail if: 
 * 
 * ESRCH        The value specified by thread does not refer to
 *              a existing thread. 
 *
 * The pthread_setschedparam() function may fail if: 
 *
 * EINVAL       The value specified by policy or one of the 
 *              scheduling parameters associated with the scheduling 
 *              policy policy is invalid. 
 *
 * ENOTSUP      An attempt was made to set the policy or scheduling 
 *              parameters to an unsupported value. 
 *
 * EPERM        The caller does not have the appropriate permission 
 *              to set either the scheduling parameters or the 
 *              scheduling policy of the specified thread. 
 *
 * EPERM        The implementation does not allow the application 
 *              to modify one of the parameters to the value specified. 
 *
 * ESRCH        The value specified by thread does not refer to 
 *              a existing thread. 
 */

EXTERN int pthread_setschedparam(pthread_t thread,
                                 int policy,
                                 const struct sched_param *restrict param);


EXTERN int pthread_attr_getschedparam(const pthread_attr_t *restrict attr,
                                      struct sched_param *restrict param);
EXTERN int pthread_attr_setschedparam(pthread_attr_t *restrict attr, 
                                      const struct sched_param *restrict param);

EXTERN int pthread_setschedprio(pthread_t thread, int prio);


/** 
 * The functions pthread_attr_setstacksize() and
 * pthread_attr_getstacksize(), respectively, set and 
 * get the thread creation stacksize attribute in the attr 
 * object. 
 * The stacksize attribute defines the minimum stack size 
 * (in bytes) allocated for the created threads stack. 
 */

EXTERN int pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize);


/** 
 * The functions pthread_attr_setstacksize() and
 * pthread_attr_getstacksize(), respectively, set and 
 * get the thread creation stacksize attribute in the attr 
 * object. 
 * The stacksize attribute defines the minimum stack size 
 * (in bytes) allocated for the created threads stack. 
 */

EXTERN int pthread_attr_getstacksize(const pthread_attr_t *restrict attr, 
                                     size_t *restrict stacksize);


/**
 * The functions pthread_attr_setstorage_np() and
 * pthread_attr_getstorage_np(), respectively, set and
 * get the storage buffer used for the thread structures in
 * the attr object.
 *
 * The storage which is passed and returned should be a
 * pointer to a struct pthread_storage_t which will last
 * for the lifetime of the thread.
 */

EXTERN int pthread_attr_setstorage_np(pthread_attr_t *attr, pthread_storage_t *storage);
EXTERN int pthread_attr_getstorage_np(pthread_attr_t *attr, pthread_storage_t **storage);

/**
 * The pthread_create() function is used to create a new thread, 
 * with attributes specified by attr, within a process. If attr is
 * NULL, the default attributes are used. If the attributes specified
 * by attr are modified later, the thread's attributes are not affected. 
 * Upon successful completion, pthread_create() stores the ID of the 
 * created thread in the location referenced by thread. 
 *
 * The thread is created executing start_routine with arg as its sole
 * argument. If the start_routine returns, the effect is as if there 
 * was an implicit call to pthread_exit() using the return value of 
 * start_routine as the exit status. Note that the thread in which 
 * main() was originally invoked differs from this. When it returns 
 * from main(), the effect is as if there was an implicit call to 
 * exit() using the return value of main() as the exit status. 
 *
 * If pthread_create() fails, no new thread is created and the 
 * contents of the location referenced by thread are undefined. 
 *
 * If successful, the pthread_create() function returns zero. 
 * Otherwise, an error number is returned to indicate the error. 
 *
 * The pthread_create() function will fail if: 
 * 
 *  EAGAIN:     The system lacked the necessary resources to create
 *              another thread, or the system-imposed limit on the
 *              total number of threads in a process 
 *              PTHREAD_THREADS_MAX would be exceeded.
 *
 *  EINVAL:     The value specified by attr is invalid. 
 *
 *  EPERM:      The caller does not have appropriate permission to 
 *              set the required scheduling parameters or scheduling 
 *              policy. 
 *
 * The pthread_create() function will not return an error code of EINTR. 
 */

EXTERN int pthread_create(pthread_t *restrict thread, 
                          const pthread_attr_t *restrict attr,
                          void *(*start)(void *), 
                          void *restrict param);

/**
 * The pthread_detach() function is used to indicate to the implementation 
 * that storage for the thread thread can be reclaimed when that thread 
 * terminates. If thread has not terminated, pthread_detach() will not 
 * cause it to terminate. The effect of multiple pthread_detach() calls 
 * on the same target thread is unspecified. 
 *
 *
 * If the call succeeds, pthread_detach() returns 0. Otherwise, an error
 * number is returned to indicate the error. 
 * 
 * The pthread_detach() function will fail if: 
 *
 *  EINVAL:     The implementation has detected that the value 
 *              specified by thread does not refer to a joinable thread. 
 *
 *  ESRCH:      No thread could be found corresponding to that specified 
 *              by the given thread ID. 
 *
 * The pthread_detach() function will not return an error code of EINTR. 
 */

EXTERN int pthread_detach(pthread_t thread);
EXTERN int pthread_equal(pthread_t t1, pthread_t t2);
EXTERN void pthread_exit(void *status);

EXTERN void pthread_ext_set_add_thread_callback(pthread_callback_t callback);
EXTERN void pthread_ext_set_delete_thread_callback(pthread_callback_t callback);


/**
 * The pthread_join() function suspends execution of the calling 
 * thread until the target thread terminates, unless the target thread 
 * has already terminated. On return from a successful pthread_join() 
 * call with a non-NULL value_ptr argument, the value passed to 
 * pthread_exit() by the terminating thread is made available in the 
 * location referenced by value_ptr. When a pthread_join() returns 
 * successfully, the target thread has been terminated. The results of 
 * multiple simultaneous calls to pthread_join() specifying the same 
 * target thread are undefined. If the thread calling pthread_join() 
 * is canceled, then the target thread will not be detached. 
 *
 * It is unspecified whether a thread that has exited but remains 
 * unjoined counts against _POSIX_THREAD_THREADS_MAX. 
 *
 * If successful, the pthread_join() function returns zero. Otherwise, 
 * an error number is returned to indicate the error. 
 *
 * The pthread_join() function will fail if: 
 *
 *  EINVAL:     The implementation has detected that the value 
 *              specified by thread does not refer to a joinable thread.
 * 
 *  ESRCH:      No thread could be found corresponding to that 
 *              specified by the given thread ID. 
 *
 * The pthread_join() function may fail if: 
 * 
 *  EDEADLK:    A deadlock was detected or the value of thread 
 *              specifies the calling thread. 
 *
 * The pthread_join() function will not return an error code of EINTR.
 */

EXTERN int pthread_join(pthread_t thread, void **value_ptr);

EXTERN int pthread_once(pthread_once_t *once_control, void (*init)(void));
EXTERN pthread_t pthread_self(void);

/**
 * The pthread_cleanup_push() function pushes the specified cancellation 
 * cleanup handler routine onto the calling thread's cancellation cleanup 
 * stack. The cancellation cleanup handler is popped from the cancellation 
 * cleanup stack and invoked with the argument arg when: (a) the thread 
 * exits (that is, calls pthread_exit()), (b) the thread acts upon a 
 * cancellation request, or (c) the thread calls pthread_cleanup_pop() 
 * with a non-zero execute argument. 
 *
 * The pthread_cleanup_pop() function removes the routine at the top of
 * the calling thread's cancellation cleanup stack and optionally 
 * invokes it (if execute is non-zero). 
 *
 * These functions may be implemented as macros and will appear as 
 * statements and in pairs within the same lexical scope (that is,
 * the pthread_cleanup_push() macro may be thought to expand to a token 
 * list whose first token is `{' with pthread_cleanup_pop() expanding
 * to a token list whose last token is the corresponding `}'. 
 *
 * The effect of calling longjmp() or siglongjmp() is undefined if 
 * there have been any calls to pthread_cleanup_push() or 
 * pthread_cleanup_pop() made without the matching call since the jump
 * buffer was filled. The effect of calling longjmp() or siglongjmp() 
 * from inside a cancellation cleanup handler is also undefined unless 
 * the jump buffer was also filled in the cancellation cleanup handler. 
 *
 * The pthread_cleanup_push() and pthread_cleanup_pop() functions 
 * return no value. 
 *
 * No errors are defined. 
 *
 * These functions will not return an error code of EINTR.
 *
 */

#define pthread_cleanup_push(routine, arg) \
	do { pthread_cleanup_t pthread_cleanup_storage_; pthread_cleanup_push_(routine, arg, &pthread_cleanup_storage_)

#define pthread_cleanup_pop(execute) \
	pthread_cleanup_pop_(execute); } while(0)

EXTERN void pthread_cleanup_push_(void (*routine)(void *), void *arg, pthread_cleanup_t *pStorage);
EXTERN void pthread_cleanup_pop_(int execute);

/**
 * Cancel states
 */
enum {
  PTHREAD_CANCEL_ENABLE,
  PTHREAD_CANCEL_DISABLE,
};


/**
 * Cancel types
 */
enum {
  PTHREAD_CANCEL_ASYNCHRONOUS,
  PTHREAD_CANCEL_DEFERRED,
};


/**
 * The pthread_setcancelstate subroutine atomically both sets the 
 * calling thread's cancelability state to the indicated state and 
 * returns the previous cancelability state at the location referenced 
 * by oldstate. Legal values for state are PTHREAD_CANCEL_ENABLE and 
 * PTHREAD_CANCEL_DISABLE.
 *
 * The cancelability state and type of any newly created threads, 
 * including the thread in which main was first invoked, are
 * PTHREAD_CANCEL_ENABLE and PTHREAD_CANCEL_DEFERRED respectively.
 */

EXTERN int pthread_setcancelstate(int state, int *oldstate);


/**
 * The pthread_setcanceltype subroutine atomically both sets the 
 * calling thread's cancelability type to the indicated type and returns
 * the previous cancelability type at the location referenced by oldtype.
 * Legal values for type are PTHREAD_CANCEL_DEFERRED and 
 * PTHREAD_CANCEL_ASYNCHRONOUS.
 */

EXTERN int pthread_setcanceltype(int type, int *oldtype);


/**
 * The pthread_testcancel subroutine creates a cancellation point in
 * the calling thread. The pthread_testcancel subroutine has no
 * effect if cancelability is disabled.
 *
 * Other cancellation points occur when calling the following 
 * subroutines: 
 *
 *      pthread_cond_wait, pthread_cond_timedwait, pthread_join 
 */

EXTERN void pthread_testcancel(void);


/**
 * The pthread_cancel() function requests that thread be canceled. 
 * The target threads cancelability state and type determines when 
 * the cancellation takes effect. When the cancellation is acted on, 
 * the cancellation cleanup handlers for thread are called. When the 
 * last cancellation cleanup handler returns, the thread-specific data 
 * destructor functions are called for thread. When the last 
 * destructor function returns, thread is terminated. 
 *
 * The cancellation processing in the target thread runs asynchronously 
 * with respect to the calling thread returning from pthread_cancel(). 
 *
 * If successful, the pthread_cancel() function returns zero. Otherwise,
 * an error number is returned to indicate the error. 
 * 
 * The pthread_cancel() function may fail if: 
 *
 *   ESRCH:	No thread could be found corresponding to that 
 *              specified by the given thread ID. 
 *
 * The pthread_cancel() function will not return an error code of EINTR.
 */

EXTERN int pthread_cancel(pthread_t thread);

/** @} */


/* ****************************************** */
/* ********** Thread specific data ********** */
/* ****************************************** */

/** @defgroup Tss Thread specific data
 *
 * @{
 */

/**
 * The pthread_key_create subroutine creates a thread-specific data 
 * key. The key is shared among all threads within the process, but each
 * thread has specific data associated with the key. The thread-specific 
 * data is a void pointer, initially set to NULL.
 * 
 * The application is responsible for ensuring that this subroutine is 
 * called only once for each requested key. This can be done, for example,
 * by calling the subroutine before creating other threads, or by using 
 * the one-time initialization facility.
 *
 * Typically, thread-specific data are pointers to dynamically allocated 
 * storage. When freeing the storage, the value should be set to NULL. 
 * It is not recommended to cast this pointer into scalar data type 
 * (int for example), because the casts may not be portable, and because 
 * the value of NULL is implementation dependent.
 *
 * An optional destructor routine can be specified. It will be called 
 * for each thread when it is terminated and detached, after the call 
 * to the cleanup routines, if the specific value is not NULL. Typically,
 * the destructor routine will release the storage thread-specific data. 
 * It will receive the thread-specific data as a parameter.
 */

EXTERN int pthread_key_create(pthread_key_t *key, void (*dtor)(void *));


/**
 * The pthread_key_delete subroutine deletes the thread-specific data 
 * key key, previously created with the pthread_key_create subroutine. 
 * The application must ensure that no thread-specific data is associated
 * with the key. No destructor routine is called.
 */

EXTERN int pthread_key_delete(pthread_key_t key);


/**
 * The pthread_getspecific function returns the value currently bound to the
 * specified key on behalf of the calling thread.
 *
 * The effect of calling pthread_setspecific or pthread_getspecific with a 
 * key value not obtained from pthread_key_create or after key has been deleted 
 * with pthread_key_delete is undefined.
 *
 * Both pthread_setspecific and pthread_getspecific may be called from a 
 * thread-specific data destructor function. However, calling 
 * pthread_setspecific from a destructor may result in lost storage or 
 * infinite loops.
 */

EXTERN void *pthread_getspecific(pthread_key_t key);


/**
 * The pthread_setspecific function associates a thread-specific value with 
 * a key obtained via a previous call to pthread_key_create. Different 
 * threads may bind different values to the same key. These values are typically 
 * pointers to blocks of dynamically allocated memory that have been reserved
 * for use by the calling thread.
 *
 * The effect of calling pthread_setspecific or pthread_getspecific with a 
 * key value not obtained from pthread_key_create or after key has been deleted 
 * with pthread_key_delete is undefined.
 *
 * Both pthread_setspecific and pthread_getspecific may be called from a 
 * thread-specific data destructor function. However, calling 
 * pthread_setspecific from a destructor may result in lost storage or 
 * infinite loops.
 */

EXTERN int pthread_setspecific(pthread_key_t key, const void *value);

/** @} */


/* ****************************** */
/* ********** Barriers ********** */
/* ****************************** */

/** @defgroup Barriers Barriers
 *
 * @{
 */

EXTERN int pthread_barrier_init(pthread_barrier_t *restrict barrier, 
                                const pthread_barrierattr_t *restrict barrierattr, 
                                unsigned int count);
EXTERN int pthread_barrier_destroy(pthread_barrier_t *barrier);
EXTERN int pthread_barrier_wait(pthread_barrier_t *barrier);

EXTERN int pthread_barrierattr_destroy(pthread_barrierattr_t *barrierattr);
EXTERN int pthread_barrierattr_init(pthread_barrierattr_t *barrierattr);

/** @} */


/* ***************************************** */
/* ********** Condition Variables ********** */
/* ***************************************** */

/** @defgroup Conditions Condition Variables
 *
 * @{
 */

EXTERN int pthread_cond_init(pthread_cond_t *restrict cond,
                             const pthread_condattr_t *restrict attr);
EXTERN int pthread_cond_broadcast(pthread_cond_t *cond);
EXTERN int pthread_cond_destroy(pthread_cond_t *cond);
EXTERN int pthread_cond_signal(pthread_cond_t *cond);
EXTERN int pthread_cond_timedwait(pthread_cond_t *restrict cond, 
                                  pthread_mutex_t *restrict mutex, 
                                  const struct timespec *restrict abstime);
EXTERN int pthread_cond_wait(pthread_cond_t *restrict cond, 
                             pthread_mutex_t *restrict mutex);

EXTERN int pthread_condattr_destroy(pthread_condattr_t *attr);
EXTERN int pthread_condattr_init(pthread_condattr_t *attr);

/** @} */


/* ***************************** */
/* ********** Mutexes ********** */
/* ***************************** */

/** @defgroup Mutexes Mutexes
 *
 * @{
 */

/**
 * The function pthread_cond_init() initializes the 
 * condition variable referenced by cond with attributes 
 * referenced by attr. If attr is NULL, the default condition
 * variable attributes are used; the effect is the same as
 * passing the address of a default condition variable attributes 
 * object. Upon successful initialisation, the state of the 
 * condition variable becomes initialized. 
 *
 * Attempting to initialize an already initialized condition 
 * variable results in undefined behaviour. 
 *
 * The function pthread_cond_destroy() destroys the given condition
 * variable specified by cond; the object becomes, in effect, 
 * uninitialized. An implementation may cause pthread_cond_destroy() 
 * to set the object referenced by cond to an invalid value. A 
 * destroyed condition variable object can be re-initialized using
 * pthread_cond_init(); the results of otherwise referencing the 
 * object after it has been destroyed are undefined. 
 *
 * It is safe to destroy an initialized condition variable upon 
 * which no threads are currently blocked. Attempting to destroy a 
 * condition variable upon which other threads are currently blocked 
 * results in undefined behaviour. 
 *
 * In cases where default condition variable attributes are 
 * appropriate, the macro PTHREAD_COND_INITIALIZER can be used to 
 * initialize condition variables that are statically allocated. 
 * The effect is equivalent to dynamic initialisation by a call to 
 * pthread_cond_init() with parameter attr specified as NULL, except 
 * that no error checks are performed. 
 *
 * If successful, the pthread_cond_init() and pthread_cond_destroy()
 * functions return zero. Otherwise, an error number is returned
 * to indicate the error. The EBUSY and EINVAL error checks, if 
 * implemented, act as if they were performed immediately at the
 * beginning of processing for the function and caused an error 
 * return prior to modifying the state of the condition variable 
 * specified by cond. 
 *
 * The pthread_cond_init() function will fail if: 
 *
 *  EAGAIN:	The system lacked the necessary resources (other
 *              than memory) to initialize another condition variable. 
 *
 *  ENOMEM:	Insufficient memory exists to initialize the 
 *              condition variable. 
 *
 * The pthread_cond_init() function may fail if: 
 *
 *  EBUSY:      The implementation has detected an attempt to
 *              re-initialize the object referenced by cond, a 
 *              previously initialized, but not yet destroyed, 
 *              condition variable. 
 *
 *  EINVAL:     The value specified by attr is invalid. 
 * 
 * The pthread_cond_destroy() function may fail if: 
 *
 *  EBUSY:      The implementation has detected an attempt to 
 *              destroy the object referenced by cond while it is 
 *              referenced (for example, while being used in a 
 *              pthread_cond_wait() or pthread_cond_timedwait()) 
 *              by another thread. 
 * 
 *  EINVAL:     The value specified by cond is invalid. 
 *
 * These functions will not return an error code of EINTR.
 */

EXTERN int pthread_mutex_destroy(pthread_mutex_t *mutex);
EXTERN int pthread_mutex_init(pthread_mutex_t *restrict mutex, 
                              const pthread_mutexattr_t *restrict attr);
EXTERN int pthread_mutex_lock(pthread_mutex_t *mutex);
EXTERN int pthread_mutex_trylock(pthread_mutex_t *mutex);
EXTERN int pthread_mutex_unlock(pthread_mutex_t *mutex);


/**
 * The pthread_mutex_timedlock() function shall lock the mutex object 
 * referenced by mutex. If the mutex is already locked, the calling 
 * thread shall block until the mutex becomes available as in the 
 * pthread_mutex_lock() function. If the mutex cannot be locked without waiting 
 * for another thread to unlock the mutex, this wait shall be terminated 
 * when the specified timeout expires.
 *
 * The timeout shall expire when the absolute time specified by abs_timeout 
 * passes, as measured by the clock on which timeouts are based (that is, 
 * when the value of that clock equals or exceeds abs_timeout), or if the 
 * absolute time specified by abs_timeout has already been passed at the 
 * time of the call.
 *
 * If the Timers option is supported, the timeout shall be based on the 
 * CLOCK_REALTIME clock; if the Timers option is not supported, the timeout
 * shall be based on the system clock as returned by the time() function. 
 *
 * The resolution of the timeout shall be the resolution of the clock on 
 * which it is based. The timespec data type is defined in the <time.h> header.
 *
 * Under no circumstance shall the function fail with a timeout if the
 * mutex can be locked immediately. The validity of the abs_timeout parameter 
 * need not be checked if the mutex can be locked immediately.
 *
 * As a consequence of the priority inheritance rules (for mutexes 
 * initialized with the PRIO_INHERIT protocol), if a timed mutex wait is
 * terminated because its timeout expires, the priority of the owner of the
 * mutex shall be adjusted as necessary to reflect the fact that this thread 
 * is no longer among the threads waiting for the mutex. 
 *
 * If successful, the pthread_mutex_timedlock() function shall return zero; 
 * otherwise, an error number shall be returned to indicate the error.
 *
 * The pthread_mutex_timedlock() function shall fail if:
 *
 * EINVAL:      The mutex was created with the protocol attribute having 
 *              the value PTHREAD_PRIO_PROTECT and the calling thread's 
 *              priority is higher than the mutex' current priority ceiling. 
 *
 * EINVAL:      The process or thread would have blocked, and the abs_timeout
 *              parameter specified a nanoseconds field value less than zero 
 *              or greater than or equal to 1000 million. 
 *
 * ETIMEDOUT:   The mutex could not be locked before the specified timeout expired. 
 *
 * The pthread_mutex_timedlock() function may fail if:
 * 
 * EINVAL:      The value specified by mutex does not refer to an 
 *              initialized mutex object. 
 *
 * EAGAIN:      The mutex could not be acquired because the maximum number
 *              of recursive locks for mutex has been exceeded.  
 *
 * EDEADLK:     A deadlock condition was detected or the current thread 
 *              already owns the mutex. 
 */

EXTERN int pthread_mutex_timedlock(pthread_mutex_t *mutex, 
                                   const struct timespec *abstime);


/**
 * The pthread_mutex_getprioceiling subroutine returns the current priority 
 * ceiling of the mutex.
 * 
 * The pthread_mutex_setprioceiling subroutine either locks the mutex if it 
 * is unlocked, or blocks until it can successfully lock the mutex, then it 
 * changes the mutex's priority ceiling and releases the mutex. When the 
 * change is successful, the previous value of the priority ceiling shall 
 * be returned in old_ceiling. The process of locking the mutex need not 
 * adhere to the priority protect protocol.
 *
 * If the pthread_mutex_setprioceiling subroutine fails, the mutex priority 
 * ceiling is not changed.
 *
 * The pthread_mutex_getprioceiling and pthread_mutex_setprioceiling 
 * subroutines can fail if:
 *
 *   EINVAL     The priority requested by the prioceiling parameter is 
 *              out of range. 
 *
 *   EINVAL     The value specified by the mutex parameter does not refer to 
 *              a currently existing mutex. 
 *
 *   ENOSYS     This function is not supported (draft 7). 
 *
 *   ENOTSUP    This function is not supported together with checkpoint/restart.
 * 
 *   EPERM      The caller does not have the privilege to perform the operation. 
 */

EXTERN int pthread_mutex_getprioceiling(const pthread_mutex_t *restrict mutex, 
                                        int *restrict prioceiling);
EXTERN int pthread_mutex_setprioceiling(pthread_mutex_t *restrict mutex, 
                                        int prioceiling, 
                                        int *restrict old_ceiling);

EXTERN int pthread_mutexattr_init(pthread_mutexattr_t *attr);
EXTERN int pthread_mutexattr_destroy(pthread_mutexattr_t *attr);

/**
 * The default value of the type attribute is PTHREAD_MUTEX_DEFAULT. 
 *
 * The type argument specifies the type of mutex. The following 
 * list describes the valid mutex types: 
 */

enum {
  /**
   * Attempting to recursively lock a mutex of this type results in 
   * undefined behavior. Attempting to unlock a mutex of this type that 
   * was not locked by the calling thread results in undefined behavior. 
   * Attempting to unlock a mutex of this type that is not locked results 
   * in undefined behavior. An implementation is allowed to map this 
   * mutex to one of the other mutex types. 
   */
#define PTHREAD_MUTEX_DEFAULT   PTHREAD_MUTEX_NORMAL

  /**
   * This type of mutex does not detect deadlock. A thread attempting
   * to relock this mutex without first unlocking the mutex deadlocks. 
   * Attempting to unlock a mutex locked by a different thread results 
   * in undefined behavior. Attempting to unlock an unlocked mutex 
   * results in undefined behavior. 
   */
  PTHREAD_MUTEX_NORMAL,

  /**
   * This type of mutex provides error checking. A thread attempting 
   * to relock this mutex without first unlocking the mutex returns an 
   * error. A thread attempting to unlock a mutex that another thread
   * has locked returns an error. A thread attempting to unlock an 
   * unlocked mutex returns an error. 
   */
  PTHREAD_MUTEX_ERRORCHECK,

  /**
   * A thread attempting to relock this mutex without first unlocking 
   * the mutex succeeds in locking the mutex. The relocking deadlock
   * that can occur with mutexes of type PTHREAD_MUTEX_NORMAL cannot 
   * occur with this type of mutex. Multiple locks of this mutex require 
   * the same number of unlocks to release the mutex before another 
   * thread can acquire the mutex. A thread attempting to unlock a mutex 
   * that another thread has locked returns an error. A thread attempting 
   * to unlock an unlocked mutex returns an error. 
   */
  PTHREAD_MUTEX_RECURSIVE,
};

EXTERN int pthread_mutexattr_gettype(const pthread_mutexattr_t *attr, int *type);
EXTERN int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type);


/**
 * The pthread_mutexattr_setprotocol() and pthread_mutexattr_getprotocol() 
 * functions, respectively, set and get the protocol attribute of a mutex
 * attribute object pointed to by attr which was previously created by the 
 * function pthread_mutexattr_init(). 
 * 
 * The protocol attribute defines the protocol to be followed in utilising
 * mutexes. The value of protocol may be one of PTHREAD_PRIO_NONE,
 * PTHREAD_PRIO_INHERIT or PTHREAD_PRIO_PROTECT.
 * 
 * When a thread owns a mutex with the PTHREAD_PRIO_NONE protocol attribute, 
 * its priority and scheduling are not affected by its mutex ownership. 
 *
 * When a thread is blocking higher priority threads because of owning one
 * or more mutexes with the PTHREAD_PRIO_INHERIT protocol attribute, it
 * executes at the higher of its priority or the priority of the highest 
 * priority thread waiting on any of the mutexes owned by this thread and 
 * initialized with this protocol. 
 *
 * When a thread owns one or more mutexes initialized with the
 * PTHREAD_PRIO_PROTECT protocol, it executes at the higher of its priority 
 * or the highest of the priority ceilings of all the mutexes owned by this
 * thread and initialized with this attribute, regardless of whether other 
 * threads are blocked on any of these mutexes or not. 
 *
 * While a thread is holding a mutex which has been initialized with the
 * PRIO_INHERIT or PRIO_PROTECT protocol attributes, it will not be subject
 * to being moved to the tail of the scheduling queue at its priority in 
 * the event that its original priority is changed, such as by a call to 
 * sched_setparam(). Likewise, when a thread unlocks a mutex that has been 
 * initialized with the PRIO_INHERIT or PRIO_PROTECT protocol attributes, 
 * it will not be subject to being moved to the tail of the scheduling 
 * queue at its priority in the event that its original priority is changed. 
 *
 * If a thread simultaneously owns several mutexes initialized with 
 * different protocols, it will execute at the highest of the priorities 
 * that it would have obtained by each of these protocols. 
 *
 * When a thread makes a call to pthread_mutex_lock(), if the symbol 
 * _POSIX_THREAD_PRIO_INHERIT is defined and the mutex was initialized with
 * the protocol attribute having the value PTHREAD_PRIO_INHERIT, when the 
 * calling thread is blocked because the mutex is owned by another thread, 
 * that owner thread will inherit the priority level of the calling thread 
 * as long as it continues to own the mutex. The implementation updates 
 * its execution priority to the maximum of its assigned priority and all 
 * its inherited priorities. Furthermore, if this owner thread itself 
 * becomes blocked on another mutex, the same priority inheritance effect 
 * will be propagated to this other owner thread, in a recursive manner. 
 */

enum {
  PTHREAD_PRIO_NONE,
  PTHREAD_PRIO_INHERIT,
  PTHREAD_PRIO_PROTECT,
};

EXTERN int pthread_mutexattr_getprotocol(const pthread_mutexattr_t *attr, 
                                         int *protocol);
EXTERN int pthread_mutexattr_setprotocol(pthread_mutexattr_t *attr, 
                                         int protocol);


/**
 * Gets and sets the prioceiling attribute of the mutex attributes object.
 *
 * The pthread_mutexattr_getprioceiling and pthread_mutexattr_setprioceiling 
 * subroutines can fail if:
 *
 *   EINVAL:    The value specified by the attr or prioceiling parameter is 
 *              invalid. 
 *   ENOSYS:    This function is not supported (draft 7). 
 *   ENOTSUP:   This function is not supported together with checkpoint/restart. 
 *   EPERM:     The caller does not have the privilege to perform the operation. 
 */

EXTERN int pthread_mutexattr_getprioceiling(const pthread_mutexattr_t *attr, 
                                            int *prioceiling);
EXTERN int pthread_mutexattr_setprioceiling(pthread_mutexattr_t *attr, 
                                            int prioceiling);


/**
 * Set and get the queueing policy for the thread waiting on the 
 * for the given mutex.  The default value is FIFO.
 *
 * The set and get functions return 0 or EINVAL
 */

enum {
  PTHREAD_QUEUE_FIFO_NP = SCE_KERNEL_ATTR_TH_FIFO,         // FIFO queue
  PTHREAD_QUEUE_PRIORITY_NP = SCE_KERNEL_ATTR_TH_PRIO,      // PRIORITY based queue
};

EXTERN int pthread_mutexattr_getqueueingpolicy_np(const pthread_mutexattr_t *attr, 
                                                  int *policy);
EXTERN int pthread_mutexattr_setqueueingpolicy_np(pthread_mutexattr_t *attr, 
                                                  int policy);

/** @} */


/* ************************************** */
/* ********** Read/Write Locks ********** */
/* ************************************** */

/** @defgroup ReadWrite Read/Write Locks
 *
 * @{
 */

/**
 * The pthread_rwlock_init() function initializes the read-write lock
 * referenced by rwlock with the attributes referenced by attr. If attr 
 * is NULL, the default read-write lock attributes are used; the effect 
 * is the same as passing the address of a default read-write lock 
 * attributes object. Once initialized, the lock can be used any 
 * number of times without being re-initialized. Upon successful 
 * initialisation, the state of the read-write lock becomes initialized
 * and unlocked. Results are undefined if pthread_rwlock_init() is called
 * specifying an already initialized read-write lock. Results are 
 * undefined if a read-write lock is used without first being initialized. 
 * If the pthread_rwlock_init() function fails, rwlock is not initialized 
 * and the contents of rwlock are undefined. 
 *
 * In cases where default read-write lock attributes are appropriate,
 * the macro PTHREAD_RWLOCK_INITIALIZER can be used to initialize 
 * read-write locks that are statically allocated. The effect is 
 * equivalent to dynamic initialisation by a call to 
 * pthread_rwlock_init() with the parameter attr specified as NULL,
 * except that no error checks are performed. 
 */

EXTERN int pthread_rwlock_init(pthread_rwlock_t *rwlock,
                               const pthread_rwlockattr_t *rwlockattr);


/** 
 * The pthread_rwlock_destroy() function destroys the read-write lock 
 * object referenced by rwlock and releases any resources used by the
 * lock. The effect of subsequent use of the lock is undefined until 
 * the lock is re-initialized by another call to pthread_rwlock_init(). 
 * An implementation may cause pthread_rwlock_destroy() to set the object 
 * referenced by rwlock to an invalid value. Results are undefined if
 * pthread_rwlock_destroy() is called when any thread holds rwlock. 
 * Attempting to destroy an uninitialized read-write lock results in 
 * undefined behaviour. A destroyed read-write lock object can 
 * be re-initialized using pthread_rwlock_init(); the results of
 * otherwise referencing the read-write lock object after it has been 
 * destroyed are undefined. 
 */

EXTERN int pthread_rwlock_destroy(pthread_rwlock_t *rwlock);


/**
 * The pthread_rwlock_rdlock() function applies a read lock to the
 * read-write lock referenced by rwlock. The calling thread acquires 
 * the read lock if a writer does not hold the lock and there are no
 * writers blocked on the lock. It is unspecified whether the 
 * calling thread acquires the lock when a writer does not hold the
 * lock and there are writers waiting for the lock. If a writer holds
 * the lock, the calling thread will not acquire the read lock. If the
 * read lock is not acquired, the calling thread blocks (that is, it
 * does not return from the pthread_rwlock_rdlock() call) until it can 
 * acquire the lock. Results are undefined if the calling thread holds 
 * a write lock on rwlock at the time the call is made. 
 *
 * Implementations are allowed to favour writers over readers to 
 * avoid writer starvation. 
 *
 * A thread may hold multiple concurrent read locks on rwlock (that 
 * is, successfully call the pthread_rwlock_rdlock() function n times).
 * If so, the thread must perform matching unlocks (that is, it must 
 * call the pthread_rwlock_unlock() function n times). 
 */

EXTERN int pthread_rwlock_rdlock(pthread_rwlock_t *rwlock);

EXTERN int pthread_rwlock_timedrdlock(pthread_rwlock_t *restrict rwlock, 
                                      const struct timespec *restrict abstime);
/**
 * The function pthread_rwlock_tryrdlock() applies a read lock as 
 * in the pthread_rwlock_rdlock() function with the exception that
 * the function fails if any thread holds a write lock on rwlock or
 * there are writers blocked on rwlock. 
 */

EXTERN int pthread_rwlock_tryrdlock(pthread_rwlock_t *rwlock);


/**
 * The pthread_rwlock_wrlock() function applies a write lock to the
 * read-write lock referenced by rwlock. The calling thread acquires
 * the write lock if no other thread (reader or writer) holds the
 * read-write lock rwlock. Otherwise, the thread blocks (that is, 
 * does not return from the pthread_rwlock_wrlock() call) until it 
 * can acquire the lock. Results are undefined if the calling thread 
 * holds the read-write lock (whether a read or write lock) at the
 * time the call is made. 
 * Implementations are allowed to favour writers over readers to 
 * avoid writer starvation. 
 */

EXTERN int pthread_rwlock_wrlock(pthread_rwlock_t *rwlock);


/**
 * The function pthread_rwlock_trywrlock() applies a write lock 
 * like the pthread_rwlock_wrlock() function, with the exception that 
 * the function fails if any thread currently holds rwlock (for 
 * reading or writing). 
 */

EXTERN int pthread_rwlock_trywrlock(pthread_rwlock_t *rwlock);

EXTERN int pthread_rwlock_timedwrlock(pthread_rwlock_t *restrict rwlock, 
                                      const struct timespec *restrict abstime);
EXTERN int pthread_rwlock_unlock(pthread_rwlock_t *rwlock);

EXTERN int pthread_rwlockattr_destroy(pthread_rwlockattr_t *rwlockattr);
EXTERN int pthread_rwlockattr_init(pthread_rwlockattr_t *rwlockattr);

/** @} */


/* ***************************************** */
/* ********** RealTime Scheduling ********** */
/* ***************************************** */

/* See <sched.h> */


/* ******************************** */
/* ********** Spin Locks ********** */
/* ******************************** */

/** @defgroup Spin Spin Locks
 *
 * @{
 */

EXTERN int pthread_spin_destroy(pthread_spinlock_t *lock);
EXTERN int pthread_spin_init(pthread_spinlock_t *lock, int pshared);
EXTERN int pthread_spin_lock(pthread_spinlock_t *lock);
EXTERN int pthread_spin_trylock(pthread_spinlock_t *lock);
EXTERN int pthread_spin_unlock(pthread_spinlock_t *lock);

/** @} */


/* ******************************************* */
/* ********** Mailbox support (_np) ********** */
/* ******************************************* */

/** @defgroup Mbx Mailbox
 *
 * @{
 */

/*
 * The mailbox mechanism is used to pass messages between
 * thread.  The messages are passed by reference, that is 
 * a pointer to the message is passed to the receiving 
 * thread.
 *
 * The pthread_mbx* functions returns similar return code than 
 * the corresponding standard functions in pthread.
 */

typedef struct pthread_mbxmsg_t
{
  //SceKernelMsgPacket header;
  char data[0]; // Place the data after the header
} pthread_mbxmsg_t;


EXTERN int pthread_mbx_destroy_np(pthread_mbx_t *mbx);
EXTERN int pthread_mbx_init_np(pthread_mbx_t *restrict mbx, 
                               const pthread_mbxattr_t *restrict attr);

EXTERN int pthread_mbx_send_np(pthread_mbx_t *mbx, 
                               pthread_mbxmsg_t *msg);

EXTERN int pthread_mbx_receive_np(pthread_mbx_t *mbx,
                                  pthread_mbxmsg_t **msg);

EXTERN int pthread_mbx_timedreceive_np(pthread_mbx_t *mbx,
                                       pthread_mbxmsg_t **msg,
                                       const struct timespec *abstime);

EXTERN int pthread_mbxattr_init_np(pthread_mbxattr_t *attr);
EXTERN int pthread_mbxattr_destroy_np(pthread_mbxattr_t *attr);

/** @} */


/* *************************************** */
/* ********** Event Flags (_np) ********** */
/* *************************************** */

/** @defgroup EventFlags Event Flags
 *
 * @{
 */

/*
 * The Event Flags mechanism is used to place a thread in a WAIT
 * state until either all the events of one of the events specified
 * by a bit in the pattern is set.
 *
 * The pthread_eventflag* functions returns similar return code than 
 * the corresponding standard functions in pthread.
 */

EXTERN int pthread_eventflag_destroy_np(pthread_eventflag_t *evtflag);
EXTERN int pthread_eventflag_init_np(pthread_eventflag_t *restrict evtflag,
                                     const pthread_eventflagattr_t *restrict attr);

EXTERN int pthread_eventflag_set_np(pthread_eventflag_t *evtflag, int pattern);

enum {
  PTHREAD_EVENTFLAG_WAITMODE_AND = SCE_KERNEL_EVENT_WAIT_MODE_AND,
  PTHREAD_EVENTFLAG_WAITMODE_OR = SCE_KERNEL_EVENT_WAIT_MODE_OR,
};

enum {
  PTHREAD_EVENTFLAG_CLEARMODE_ALL = SCE_KERNEL_EVF_WAITMODE_CLEAR_ALL,
  PTHREAD_EVENTFLAG_CLEARMODE_PAT = SCE_KERNEL_EVF_WAITMODE_CLEAR_PAT,
};

EXTERN int pthread_eventflag_wait_np(pthread_eventflag_t *evtflag, 
                                     unsigned int pattern,
                                     int waitmode,
                                     int clearmode,
                                     unsigned int *result);

EXTERN int pthread_eventflag_try_np(pthread_eventflag_t *evtflag, 
                                    unsigned int pattern,
                                    int waitmode,
                                    int clearmode,
                                    unsigned int *result);

EXTERN int pthread_eventflag_timedwait_np(pthread_eventflag_t *evtflag, 
                                          unsigned int pattern,
                                          int waitmode,
                                          int clearmode,
                                          const struct timespec *abstime,
                                          unsigned int *result);

EXTERN int pthread_eventflagattr_init_np(pthread_eventflagattr_t *attr);
EXTERN int pthread_eventflagattr_destroy_np(pthread_eventflagattr_t *attr);

/** @} */


/* **************************************** */
/* ********** Message Pipe (_np) ********** */
/* **************************************** */

/** @defgroup Pipe Message Pipe
 *
 * @{
 */

/*
 * The message pipe mechanism is similar to the mailbox mechanism
 * except that the data is passed by copy through a pipe.
 */

EXTERN int pthread_msgpipe_destroy_np(pthread_msgpipe_t *msgpipe);
EXTERN int pthread_msgpipe_init_np(pthread_msgpipe_t *restrict msgpipe,
                                   const pthread_msgpipeattr_t *restrict attr);

enum {
  PTHREAD_MSGPIPE_WAITMODE_FULL = 0x00000001,
  PTHREAD_MSGPIPE_WAITMODE_ASAP = 0x00000000,
};

EXTERN int pthread_msgpipe_send_np(pthread_msgpipe_t *msgpipe, 
                                   void *buffer,
                                   int size,
                                   int waitmode,
                                   int *result);

EXTERN int pthread_msgpipe_trysend_np(pthread_msgpipe_t *msgpipe, 
                                      void *buffer,
                                      int size,
                                      int waitmode,
                                      int *result);

EXTERN int pthread_msgpipe_timedsend_np(pthread_msgpipe_t *msgpipe, 
                                        void *buffer,
                                        int size,
                                        int waitmode,
                                        const struct timespec *abstime,
                                        int *result);

EXTERN int pthread_msgpipe_receive_np(pthread_msgpipe_t *msgpipe, 
                                      void *buffer,
                                      int size,
                                      int waitmode,
                                      int *result);

EXTERN int pthread_msgpipe_tryreceive_np(pthread_msgpipe_t *msgpipe, 
                                         void *buffer,
                                         int size,
                                         int waitmode,
                                         int *result);

EXTERN int pthread_msgpipe_timedreceive_np(pthread_msgpipe_t *msgpipe, 
                                           void *buffer,
                                           int size,
                                           int waitmode,
                                           const struct timespec *abstime,
                                           int *result);

EXTERN int pthread_msgpipeattr_init_np(pthread_msgpipeattr_t *attr);
EXTERN int pthread_msgpipeattr_destroy_np(pthread_msgpipeattr_t *attr);

/** @} */


/* ********************************************************* */
/* ********** Other Non-Portable functions (*_np) ********** */
/* ********************************************************* */

/** @defgroup Others Other functions
 *
 * @{
 */

/**
 * Return the number of processors for this hardware.
 * Always 1 on the PSP
 */

EXTERN int pthread_num_processors_np();


/** 
 * Put the current thread in WAIT state for the specified
 * amount of time
 */

EXTERN int pthread_sleep_np(int usecond);


/** 
 * Put the current thread in WAIT state for the specified
 * amount of time
 */

EXTERN int pthread_delay_np(const struct timespec *time);


/** 
 * Put the current thread in WAIT state for the specified
 * amount of time
 */

EXTERN int pthread_delay_until_np(const struct timespec *abstime);


/** 
 * Get the thread ID of the given thread
 * Return 0 (success) or EINVAL
 */

EXTERN int pthread_getthreadid_np(pthread_t thread, int *id);


/**
 * Gets elapsed time from system operation start.
 * Always return 0 (success)
 */

EXTERN int pthread_getsystemtime_np(struct timespec *t);


/* 
 * Does nothing for now.  Returns 0
 */

EXTERN void *pthread_timechange_handler_np (void *arg);

/** @} */


/* ********************************************* */
/* ********** Unimplemented functions ********** */
/* ********************************************* */

/*
EXTERN int pthread_atfork(void (*)(void), void (*)(void), void(*)(void));
EXTERN int pthread_attr_getguardsize(const pthread_attr_t *restrict attr, 
                                     size_t *restrict guardsize);
EXTERN int pthread_attr_getinheritsched(const pthread_attr_t *restrict, int *restrict);
EXTERN int pthread_attr_getschedpolicy(const pthread_attr_t *restrict restrict, 
                                       int *restrict restrict);
EXTERN int pthread_attr_setschedpolicy(pthread_attr_t *restrict attr, int value);
EXTERN int pthread_attr_getscope(const pthread_attr_t *restrict , int *restrict );
EXTERN int pthread_attr_getstack(const pthread_attr_t restrict *, void restrict **, size_t restrict *);
EXTERN int pthread_attr_getstackaddr(const pthread_attr_t *restrict, void **restrict);
EXTERN int pthread_attr_setguardsize(pthread_attr_t *attr, size_t guardsize);
EXTERN int pthread_attr_setinheritsched(pthread_attr_t *, int);
EXTERN int pthread_attr_setscope(pthread_attr_t *, int);
EXTERN int pthread_attr_setstack(pthread_attr_t *, void *, size_t);
EXTERN int pthread_attr_setstackaddr(pthread_attr_t *, void *);
EXTERN int pthread_barrierattr_getpshared(const pthread_barrierattr_t *barrierattr, int *barrier);
EXTERN int pthread_barrierattr_setpshared(pthread_barrierattr_t *barrier, int value);
EXTERN int pthread_condattr_getclock(const pthread_condattr_t *attr, clockid_t *c);
EXTERN int pthread_condattr_getpshared(const pthread_condattr_t *attr, int *val);
EXTERN int pthread_condattr_setclock(pthread_condattr_t *attr, clockid_t c);
EXTERN int pthread_condattr_setpshared(pthread_condattr_t *attr, int value);
EXTERN int pthread_getconcurrency(void);
EXTERN int pthread_getcpuclockid(pthread_t, clockid_t *);
EXTERN int pthread_mutexattr_getpshared(const pthread_mutexattr_t *, int *);
EXTERN int pthread_mutexattr_setpshared(pthread_mutexattr_t *, int);
EXTERN int pthread_rwlockattr_getpshared(const pthread_rwlockattr_t *restrict rwlockattr, 
                                         int *restrict value);
EXTERN int pthread_rwlockattr_setpshared(pthread_rwlockattr_t *rwlock, int value);
EXTERN int pthread_setconcurrency(int);
*/

#endif /* _H_pthread_psp */
