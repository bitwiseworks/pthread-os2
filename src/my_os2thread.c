 
#define INCL_DOS
#define INCL_DOSERRORS
#include <os2.h>

#include <stdlib.h>
#include <string.h>
#include <process.h>
#include <types.h>
#include <sys/builtin.h>

#include "pthread.h"

static pthread_mutex_t THR_LOCK_thread = PTHREAD_MUTEX_INITIALIZER;

static pthread_key_t THR_self = -1;

struct pthread_map
{
  ULONG		  hThread;
  pthread_handler func;
  void *	  param;
  void *	  rc;
  BOOL            done;
  int             detachState;
  pthread_mutex_t cancelLock;	/* Used for async-cancel safety */
};


static pthread_handler_decl(pthread_start,param)
{
  struct pthread_map* map = (struct pthread_map*) param;
  pthread_handler func = map->func;
  void *func_param = map->param;

  // store data structure pointer in thread self memory
  pthread_setspecific(THR_self, map);

  // wait for beginthread to return
  pthread_mutex_lock(&THR_LOCK_thread);
  pthread_mutex_unlock(&THR_LOCK_thread);
  
  // thread about to start
  map->rc = (*func)(func_param);
  // terminate this thread and free resources
  pthread_exit( map->rc);

  // should never get here!
  return 0;				  /* Safety */
}

int pthread_create(pthread_t *thread_id, pthread_attr_t *attr,
		   pthread_handler func, void *param)
{
  struct pthread_map *map;

  if (!THR_LOCK_thread)
    if (pthread_mutex_init(&THR_LOCK_thread,NULL)==-1)
      return -1; // fail
 
  if (THR_self == -1)
    if (pthread_key_create(&THR_self,NULL)==-1)
      return -1; // fail

  // allocate thread private data structure
  map=(struct pthread_map *)calloc(1,sizeof(*map));
  if (!map)
    return(-1);

  memset( map, 0, sizeof(struct pthread_map));
  map->func=func;
  map->param=param;
  map->detachState = PTHREAD_CREATE_JOINABLE;
  map->cancelLock = PTHREAD_MUTEX_INITIALIZER;
  // set thread attributes
  if (attr != NULL) {
    map->detachState = attr->detachstate;
  }

  pthread_mutex_lock(&THR_LOCK_thread);

  map->hThread=(ULONG)_beginthread((void( *)(void *)) pthread_start, NULL,
			        attr ? (attr->dwStackSize ? attr->dwStackSize : 65535) : 65535, 
				(void*) map);
  // return pointer to caller
  *thread_id = map;
  // set thread priority
  if (attr != NULL) {
    pthread_setprio(map->hThread, attr->priority);
  }

  // data initialized, allow thread to run
  pthread_mutex_unlock(&THR_LOCK_thread);

  if (map->hThread == (ULONG) -1) {
    int error=errno;
    free(map);
    return(error ? error : -1);
  }

  // ok
  return(0);
}


void pthread_exit(void *a)
{
  // get data structure pointer from thread self memory
  struct pthread_map* map = (struct pthread_map*) pthread_getspecific(THR_self);

  // thread has ended, pthread_join is not allowed to wait for thread
  map->done = TRUE;
  map->rc = a;

  // free resources for detached threads
  if (map->detachState == PTHREAD_CREATE_DETACHED) {
    free( map);
    pthread_setspecific(THR_self, NULL);
  }

  // thread is joinable, pthread_join is supposed to be called from main thread

  // let libc terminate this thread
  _endthread();
}

void pthread_setprio( int a, int b)
{
   DosSetPriority(PRTYS_THREAD,PRTYC_NOCHANGE, b, a);   
}

int pthread_dummy(int ret)
{
  return ret;
}

pthread_t
pthread_self (void)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      This function returns a reference to the current running
      *      thread.
      *
      * PARAMETERS
      *      N/A
      *
      *
      * DESCRIPTION
      *      This function returns a reference to the current running
      *      thread.
      *
      * RESULTS
      *              pthread_t       reference to the current thread
      *
      * ------------------------------------------------------
      */
{
  struct pthread_map* self;

  self = (struct pthread_map*) pthread_getspecific(THR_self);
  if (self == NULL)
    {
      /*
       * Need to create an implicit 'self' for the currently
       * executing thread.
       */
      self=(struct pthread_map *)calloc(1,sizeof(*self));
      if (self != NULL)
	{
	  /*
	   * This is a non-POSIX thread which has chosen to call
	   * a POSIX threads function for some reason. We assume that
	   * it isn't joinable, but we do assume that it's
	   * (deferred) cancelable.
	   */
	  self->detachState = PTHREAD_CREATE_DETACHED;
	  self->hThread = _gettid();
          pthread_setspecific(THR_self, self);
	}
    }

  return (self);

}				/* pthread_self */

/* 
 * pthread_join - Causes the calling thread to wait for the termination of a specified thread
*/
int pthread_join( pthread_t thread, pthread_addr_t *status)
{
  struct pthread_map* self = (struct pthread_map*) pthread_self();
  // get data structure
  struct pthread_map* map = (struct pthread_map*) thread;
  APIRET rc;

  // not valid for same thread
  if (self == thread) 
      return EDEADLK;

  // not valid for detached thread
  if (map->detachState == PTHREAD_CREATE_DETACHED)
      return EINVAL;

  // now wait for thread end
  if (!map->done) {
    rc = DosWaitThread(&map->hThread, DCWW_WAIT);
    if (rc == ERROR_INVALID_THREADID)
      return ESRCH;
    if (rc != NO_ERROR)
      return EINVAL;
  }

  // thread ended, get value
  if (status != NULL)
    *status = map->rc;

  // free resources
  free( map);

  return 0;
}

void pthread_cleanup_push(void (*routine) (void *), void *routine_arg)
{
}

void pthread_cleanup_pop(int execute)
{
}

int pthread_once(pthread_once_t *once_control, void (*init_routine)(void))
{
  if (once_control == NULL || init_routine == NULL)
    {
      return EINVAL;
    }

  if (!__atomic_xchg((unsigned*)&once_control->done, 0)) /* MBR fence */
    {
      pthread_mutex_lock(&once_control->lock);

      if (!once_control->done)
	{
	  (*init_routine)();
	  once_control->done = PTW32_TRUE;
	}

	pthread_mutex_unlock(&once_control->lock);
    }

  return 0;
}

void pthread_yield(void)
{
    DosSleep(0);
}

int pthread_setcancelstate(int state, int *oldstate)
{
    return(0);
}

int pthread_setcanceltype(int type, int *oldtype)
{
    return(0);
}

void pthread_testcancel(void)
{
    return;
}

int pthread_atfork(void (*prepare)(void), void (*parent)(void),void (*child)(void))
{
    return(0);
}


int
pthread_detach (pthread_t thread)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      This function detaches the given thread.
      *
      * PARAMETERS
      *      thread
      *              an instance of a pthread_t
      *
      *
      * DESCRIPTION
      *      This function detaches the given thread. You may use it to
      *      detach the main thread or to detach a joinable thread.
      *      NOTE:   detached threads cannot be joined;
      *              storage is freed immediately on termination.
      *
      * RESULTS
      *              0               successfully detached the thread,
      *              EINVAL          thread is not a joinable thread,
      *              ENOSPC          a required resource has been exhausted,
      *              ESRCH           no thread could be found for 'thread',
      *
      * ------------------------------------------------------
      */
{
  // get data structure
  struct pthread_map* map = (struct pthread_map*) thread;
  int result;
#if 0
  BOOL destroyIt = PTW32_FALSE;
#endif

  DosEnterCritSec();

  if (NULL == map)
    {
      result = ESRCH;
    }
  else if (PTHREAD_CREATE_DETACHED == map->detachState)
    {
      result = EINVAL;
    }
  else
    {
      /*
       * Joinable ptw32_thread_t structs are not scavenged until
       * a join or detach is done. The thread may have exited already,
       * but all of the state and locks etc are still there.
       */
      result = 0;

      if (pthread_mutex_lock (&map->cancelLock) == 0)
	{
	  map->detachState = PTHREAD_CREATE_DETACHED;
#if 0 // YD state not implemented
	  if (map->state != PThreadStateLast)
	    {
	      map->detachState = PTHREAD_CREATE_DETACHED;
	    }
	  else if (map->detachState != PTHREAD_CREATE_DETACHED)
	    {
	      /*
	       * Thread is joinable and has exited or is exiting.
	       */
	      destroyIt = PTW32_TRUE;
	    }
#endif
	  (void) pthread_mutex_unlock (&map->cancelLock);
	}
      else
	{
	  /* cancelLock shouldn't fail, but if it does ... */
	  result = ESRCH;
	}
    }

  DosExitCritSec();

#if 0 // YD state not implemented
  if (result == 0)
    {
      /* Thread is joinable */

      if (destroyIt)
	{
	  /* The thread has exited or is exiting but has not been joined or
	   * detached. Need to wait in case it's still exiting.
	   */
	  (void) WaitForSingleObject(tp->threadH, INFINITE);
	  ptw32_threadDestroy (thread);
	}
    }
#endif

  return (result);

}				/* pthread_detach */

int
pthread_kill (pthread_t thread, int sig)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      This function requests that a signal be delivered to the
      *      specified thread. If sig is zero, error checking is
      *      performed but no signal is actually sent such that this
      *      function can be used to check for a valid thread ID.
      *
      * PARAMETERS
      *      thread  reference to an instances of pthread_t
      *      sig     signal. Currently only a value of 0 is supported.
      *
      *
      * DESCRIPTION
      *      This function requests that a signal be delivered to the
      *      specified thread. If sig is zero, error checking is
      *      performed but no signal is actually sent such that this
      *      function can be used to check for a valid thread ID.
      *
      * RESULTS
      *              ESRCH           the thread is not a valid thread ID,
      *              EINVAL          the value of the signal is invalid
      *                              or unsupported.
      *              0               the signal was successfully sent.
      *
      * ------------------------------------------------------
      */
{
  int result = 0;

  DosEnterCritSec();

  // get data structure
  struct pthread_map* map = (struct pthread_map*) thread;

  if (NULL == map
      || TRUE == map->done
      || 0 == map->hThread)
    {
      result = ESRCH;
    }

  DosExitCritSec();

  if (0 == result && 0 != sig)
    {
      /*
       * Currently does not support any signals.
       */
      result = EINVAL;
    }

  return result;

}				/* pthread_kill */
