 
#define INCL_DOS
#define INCL_DOSERRORS
#define INCL_EXAPIS
#define INCL_EXAPIS_MAPPINGS
#include <os2emx.h>

#include <stdlib.h>
#include <string.h>
#include <process.h>
#include <types.h>
#include <sys/builtin.h>

#define INCL_LIBLOADEXCEPTQ
#define INCL_FORKEXCEPTQ
#include "exceptq.h"

//#define DEBUG

#include "pthread.h"
#include "pthread_private.h"

static pthread_mutex_t THR_LOCK_thread = PTHREAD_MUTEX_INITIALIZER;

static pthread_key_t THR_self = (pthread_key_t)-1;

/**
 * Initiates the pending structures data.
 *
 * @returns 0 on success.
 * @returns -1 on failure.
 */
int _pthreadInit(void)
{
	if (THR_LOCK_thread >= PTHREAD_ERRORCHECK_MUTEX_INITIALIZER)
		if (pthread_mutex_init(&THR_LOCK_thread,NULL)==-1)
			return -1; // fail

	if (THR_self == (pthread_key_t)-1)
		if (pthread_key_create(&THR_self,NULL)==-1)
			return -1; // fail
			
	return 0;
}

#ifndef NO_DYNAMIC_LIB
int  _CRT_init(void);
void _CRT_term(void);
void __ctordtorInit(void);
void __ctordtorTerm(void);

/**
 * initialize global dll loading/unloading.
 *
 * @returns 1 on success.
 * @returns 0 on failure.
 */
unsigned long _System _DLL_InitTerm(unsigned long hModule, unsigned long ulFlag)
{
	APIRET rc;

	switch (ulFlag) {
	case 0:
	{
		if (_CRT_init() != 0)
		return 0UL;
		__ctordtorInit();
		
		// init
		_pthreadInit();
		break;
	}

	case 1:
	{
		__ctordtorTerm();
		_CRT_term();
		break;
	}

	default:
		return 0UL;
	}

	// success
	return 1UL;
}
#endif

/*
 *
*/
static pthread_handler_decl(pthread_start,param)
{
	EXCEPTIONREGISTRATIONRECORD exRegRec;
	pthread_t thread = (pthread_t) param;
	pthread_handler func = thread->func;
	void *func_param = thread->param;

	// install exception handler (dynamically loaded)
	if (LibLoadExceptq(&exRegRec)) {
		thread->pExRegRec = &exRegRec;
	}

	// store data structure pointer in thread self memory
	pthread_setspecific(THR_self, thread);

	// wait for beginthread to return
	pthread_mutex_lock(&THR_LOCK_thread);
	pthread_mutex_unlock(&THR_LOCK_thread);

	// thread about to start
	thread->rc = (*func)(func_param);
	// terminate this thread and free resources
	pthread_exit( thread->rc);

	// should never get here!
	return 0;				  /* Safety */
}

int pthread_create(pthread_t *thread_id, const pthread_attr_t *attr,
		   pthread_handler func, void *param)
{
	pthread_t thread;
	int stackSize = 0;

	if (THR_LOCK_thread >= PTHREAD_ERRORCHECK_MUTEX_INITIALIZER)
		_pthreadInit();

	// allocate thread private data structure
	thread = (pthread_t)calloc(1,sizeof(*thread));
	if (!thread)
		return(-1);

	thread->func=func;
	thread->param=param;
	thread->detachState = PTHREAD_CREATE_JOINABLE;
	thread->cancelLock = PTHREAD_MUTEX_INITIALIZER;
	// set thread attributes
	if (attr != NULL) {
		thread->detachState = (*attr)->detachstate;
	}

	pthread_mutex_lock(&THR_LOCK_thread);

	if (attr != NULL)
		stackSize = (*attr)->dwStackSize;
	stackSize = (stackSize > PTHREAD_STACK_DEFAULT 
				 ? stackSize : PTHREAD_STACK_DEFAULT);

	thread->hThread=(ULONG)_beginthread((void( *)(void *)) pthread_start, NULL,
										stackSize, (void*) thread);

	// return pointer to caller
	*thread_id = thread;

	// set thread priority
	if (attr != NULL) {
		pthread_setprio(thread->hThread, (*attr)->priority);
	}

	// data initialized, allow thread to run
	pthread_mutex_unlock(&THR_LOCK_thread);

	if (thread->hThread == (ULONG) -1) {
		int error=errno;
		free(thread);
		return(error ? error : -1);
	}

	// ok
	return(0);
}


void pthread_exit(void *a)
{
	// get data structure pointer from thread self memory
	pthread_t thread = (pthread_t) pthread_getspecific(THR_self);
	EXCEPTIONREGISTRATIONRECORD* pExRegRec = thread->pExRegRec;
	
	// thread has ended, pthread_join is not allowed to wait for thread
	thread->done = TRUE;
	thread->rc = a;
	
	// free resources for detached threads
	if (thread->detachState == PTHREAD_CREATE_DETACHED) {
		// free resources
		free( thread);
		pthread_setspecific(THR_self, NULL);
	}

	// thread is joinable, pthread_join is supposed to be called from main thread


	// remove exception handler
	if (pExRegRec) {
		UninstallExceptq(pExRegRec);
	}

	// let libc terminate this thread
	_endthread();
}

int pthread_equal(pthread_t t1,pthread_t t2)
{
	return ( t1 == t2);
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
	pthread_t thread;

	if (THR_LOCK_thread >= PTHREAD_ERRORCHECK_MUTEX_INITIALIZER)
		_pthreadInit();
	
	thread = (pthread_t) pthread_getspecific(THR_self);
	if (thread == NULL)
	{
		/*
		* Need to create an implicit 'self' for the currently
		* executing thread.
		*/
		thread=(pthread_t)calloc(1,sizeof(*thread));
		if (thread != NULL)
		{
			/*
			* This is a non-POSIX thread which has chosen to call
			* a POSIX threads function for some reason. We assume that
			* it isn't joinable, but we do assume that it's
			* (deferred) cancelable.
			*/
			thread->detachState = PTHREAD_CREATE_DETACHED;
			thread->hThread = _gettid();
			pthread_setspecific(THR_self, thread);
		}
	}
	
	return (thread);
} /* pthread_self */

/* 
 * pthread_join - Causes the calling thread to wait for the termination of a specified thread
*/
int pthread_join( pthread_t thread, void **status)
{
	pthread_t self = (pthread_t) pthread_self();
	// get data structure
	pthread_t map = (pthread_t) thread;
	APIRET rc;

	// not valid for same thread
	if (self == thread) 
		return EDEADLK;

	// not valid for detached thread
	if (map->detachState == PTHREAD_CREATE_DETACHED)
		return EINVAL;

	// now wait for thread end
#ifdef DEBUG
	printf( "(#%d) pthread_join map->done %d, map->hThread %d\n", _gettid(), map->done, map->hThread);
#endif
	if (!map->done) {
		rc = DosWaitThread(&map->hThread, DCWW_WAIT);
#ifdef DEBUG
	printf( "DosWaitThread rc %d, map->hThread %d\n", rc, map->hThread);
#endif
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

	if (__atomic_cmpxchg32((unsigned*)&once_control->done, 0, 0)) /* MBR fence */
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

int pthread_yield(void)
{
	DosSleep(0);
	return(0);
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
	pthread_t map = (pthread_t) thread;
	int result;
#if 0
	BOOL destroyIt = PTW32_FALSE;
#endif
	
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

	// get data structure
	pthread_t map = (pthread_t) thread;

	if (NULL == map
	  || TRUE == map->done
	  || 0 == map->hThread)
	{
		result = ESRCH;
	}

	if (0 == result && 0 != sig)
	{
		/*
		* Currently does not support any signals.
		*/
		result = EINVAL;
	}

	return result;
}				/* pthread_kill */

int
pthread_cancel (pthread_t thread)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      This function requests cancellation of 'thread'.
      *
      * PARAMETERS
      *      thread
      *              reference to an instance of pthread_t
      *
      *
      * DESCRIPTION
      *      This function requests cancellation of 'thread'.
      *      NOTE: cancellation is asynchronous; use pthread_join to
      *                wait for termination of 'thread' if necessary.
      *
      * RESULTS
      *              0               successfully requested cancellation,
      *              ESRCH           no thread found corresponding to 'thread',
      *              ENOMEM          implicit self thread create failed.
      * ------------------------------------------------------
      */
{
	int result;
	int cancel_self;
	pthread_t self;

	result = pthread_kill (thread, 0);

	if (0 != result)
	{
		return result;
	}

	if ((self = pthread_self ()) == NULL)
	{
		return ENOMEM;
	};

	return (result);
}

