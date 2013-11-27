//Sony Computer Entertainment Confidential
#include "pthread/include/pthread.h"
#include <string.h> // memset

#include <errno.h>
#include <stdlib.h>

typedef struct
{
  int count;
  pthread_mutex_t mutex;
  void (*dtor)(void*);  
} pthread_key;


static pthread_key key_table[PTHREAD_KEYS_MAX];
static pthread_mutex_t key_mutex = PTHREAD_MUTEX_INITIALIZER;

static pthread_once_t init_once_control = PTHREAD_ONCE_INIT;
static void init()
{
  memset(key_table, 0, sizeof(key_table));
}


/*
 * The pthread_key_create subroutine creates a thread-specific 
 * data key. The key is shared among all threads within the 
 * process, but each thread has specific data associated with 
 * the key. The thread-specific data is a void pointer, 
 * initially set to NULL.
 *
 * The application is responsible for ensuring that this 
 * subroutine is called only once for each requested key. This
 * can be done, for example, by calling the subroutine before 
 * creating other threads, or by using the one-time 
 * initialization facility.
 *
 * Typically, thread-specific data are pointers to dynamically
 * allocated storage. When freeing the storage, the value
 * should be set to NULL. It is not recommended to cast this
 * pointer into scalar data type (int for example), because the
 * casts may not be portable, and because the value of NULL is
 * implementation dependent.
 *
 * An optional destructor routine can be specified. It will be
 * called for each thread when it is terminated and detached,
 * after the call to the cleanup routines, if the specific
 * value is not NULL. Typically, the destructor routine will
 * release the storage thread-specific data. It will receive
 * the thread-specific data as a parameter.
 */

EXTERN int pthread_key_create(pthread_key_t *key, void (*dtor)(void *))
{
  int kix;
  int res;

  pthread_once(&init_once_control, init);

  pthread_mutex_lock(&key_mutex);
  for (kix = 0; kix < PTHREAD_KEYS_MAX; kix++) 
    if (key_table[kix].count == 0) 
      {
        key_table[kix].count++;
        key_table[kix].dtor = dtor;
        pthread_mutex_init(&(key_table[kix].mutex), NULL);
        key->key = kix;
        res = 0;
        goto exit;
      }

  res = EAGAIN;
 exit:
  pthread_mutex_unlock(&key_mutex);
  return res;
}


/*
 * The pthread_key_delete subroutine deletes the thread-specific
 * data key key, previously created with the pthread_key_create
 * subroutine. The application must ensure that no 
 * thread-specific data is associated with the key. No
 * destructor routine is called.
 */

EXTERN int pthread_key_delete(pthread_key_t key)
{
  int k = key.key;
  int c;

  if (k < 0 || k >= PTHREAD_KEYS_MAX) 
    return EINVAL;

  pthread_mutex_lock(&(key_table[k].mutex));
  if ((c = key_table[k].count) == 0)
    goto exit;
  
  if (c == 1)
    {
      // The key is there but there is no thread specific 
      // data associated to it.  We can safely delete
      key_table[k].dtor = NULL;
      key_table[k].count = 0;
      goto exit;
    }

  // We could delete the key if we knew where is the tsd.
  // If we just delete the key and then reallocate it, 
  // we will end up with obsolete tsd

 exit:
  pthread_mutex_destroy(&(key_table[k].mutex));
  return 0;
}


EXTERN void pthread_cleanupspecific_(pthread_t me) 
{
  void * data;
  int key;
  int iter;

  pthread_mutex_lock(&key_mutex);

  for (iter = 0; iter < PTHREAD_DESTRUCTOR_ITERATIONS; iter++)
    for (key = 0; key < PTHREAD_KEYS_MAX; key++) 
      {
        if (me->specific_data_count) 
          {
            if (me->specific_data[key] == NULL)
              continue;

            data = (void *)me->specific_data[key];
            me->specific_data[key] = NULL;
            me->specific_data_count--;
            if (key_table[key].dtor) 
              {
                pthread_mutex_unlock(&key_mutex);
                key_table[key].dtor(data);
                pthread_mutex_lock(&key_mutex);
              }
            key_table[key].count--;
          } 
        else 
          goto exit;
      }
  
 exit:
  pthread_mutex_unlock(&key_mutex);
}


/*
 * Returns and sets the thread-specific data associated with the
 * specified key.
 */

EXTERN int pthread_setspecific(pthread_key_t key, const void * value)
{
  pthread_t me = pthread_self();
  int k = key.key;

  if (k < 0 || k >= PTHREAD_KEYS_MAX)
    return EINVAL;

  pthread_mutex_lock(&(key_table[k].mutex));
  if (key_table[k].count == 0)
    goto exit;

  if (me->specific_data[k] == NULL) 
    {
      if (value != NULL) 
        {
          me->specific_data_count++;
          key_table[k].count++;
        }
    } 
  else 
    {
      if (value == NULL) 
        {
          me->specific_data_count--;
          key_table[k].count--;
        }
    }
  me->specific_data[k] = value;

 exit:
  pthread_mutex_unlock(&(key_table[k].mutex));
  return 0;
}


EXTERN void * pthread_getspecific(pthread_key_t key)
{
  void *ret;
  pthread_t me = pthread_self();
  int k = key.key;

  if (me->specific_data == NULL ||
      k < 0 || k >= PTHREAD_KEYS_MAX)
    return NULL;

  //pthread_mutex_lock(&(key_table[k].mutex)); //>t Auday: no need for locking here
  if (key_table[k].count) 
    {
      ret = (void *)me->specific_data[k];
    } 
  else 
    {
      ret = NULL;
    }
 // pthread_mutex_unlock(&(key_table[k].mutex)); //>t Auday: no need for locking here

  return(ret);
}
