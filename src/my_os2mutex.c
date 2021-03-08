/*
 * Copyright (c) 1995 John Birrell <jb@cimlogic.com.au>.
 * All rights reserved.
 *
 * Modified and extended by Antony T Curtis <antony.curtis@olcs.net>
 * for use with OS/2.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by John Birrell.
 * 4. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY JOHN BIRRELL AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#define INCL_DOS
#define INCL_DOSERRORS
#define INCL_EXAPIS
#define INCL_EXAPIS_MAPPINGS
#include <os2emx.h>

#include <sys/smutex.h>

#include <stdlib.h>
#include <errno.h>
#include <InnoTekLIBC/errno.h>
#include <InnoTekLIBC/fork.h>

#include "pthread.h"
#include "pthread_private.h"

/*
 * Global variables. NOTE: must be reset to initial values in forkChild below.
 */

static _smutex mutex_init_lock = 0;

/*
 * End of global variables.
 */

static int _pthread_mutex_lazy_init(pthread_mutex_t * mutex)
{
	int rc = 0;
	struct pthread_mutexattr_t_ attr_ = {0};
	pthread_mutexattr_t attr = &attr_;

	_smutex_request(&mutex_init_lock);
	/*
	 * Note that we also check for NULL as on *nix pthread_mutex_t is a struct and
	 * some apps don't use PTHREAD_MUTEX_INITIALIZER assuming that the struct
	 * located in a global data segment is initialized to zeroes by the compiler.
	 * In either case, we return EINVAL as per POSIX.
	 */
	if (!*mutex || *mutex >= PTHREAD_ERRORCHECK_MUTEX_INITIALIZER) {
		if (!*mutex || *mutex == PTHREAD_MUTEX_INITIALIZER)
			attr_.kind = PTHREAD_MUTEX_DEFAULT;
		else if (*mutex == PTHREAD_RECURSIVE_MUTEX_INITIALIZER)
			attr_.kind = PTHREAD_MUTEX_RECURSIVE;
		else if (*mutex == PTHREAD_ERRORCHECK_MUTEX_INITIALIZER)
			attr_.kind = PTHREAD_MUTEX_ERRORCHECK;
		rc = pthread_mutex_init(mutex, &attr);
	}
	_smutex_release(&mutex_init_lock);

	return rc;
}

int
pthread_mutex_init(pthread_mutex_t * mutex,
		   const pthread_mutexattr_t * mutex_attr)
{
#if !defined(USE_FMUTEX)
	APIRET		rc = 0;
#endif
	pthread_mutex_t mx;

	if (mutex == NULL)
		return EINVAL;

	mx = (pthread_mutex_t) calloc(1, sizeof (*mx));

	if (mx == NULL)
		return ENOMEM;

	mx->recursive_count = 0;

	if (!mutex_attr || !*mutex_attr) {
		mx->kind = PTHREAD_MUTEX_DEFAULT;
	} else {
		/* TODO: Implement PTHREAD_MUTEX_ERRORCHECK via checked _fmutex */
		mx->kind = (*mutex_attr)->kind;
	}

#if defined(USE_FMUTEX)
	/* Postpone OS/2 sync primitive creation till it's really needed */
	mx->fmutex = (_fmutex)_FMUTEX_INITIALIZER_EX(0, "pthread_mutex_t");
#else
	rc = DosCreateMutexSem(NULL,(PHMTX)&mx->sem,0,0);
	if (rc)
		return __libc_native2errno(rc);
#endif

	*mutex = mx;

	/* Return the completion status: */
	return (0);
}

int
pthread_mutex_destroy(pthread_mutex_t * mutex)
{
#if !defined(USE_FMUTEX)
	APIRET		rc = 0;
#endif
	pthread_mutex_t mx;

	if (!mutex)
		return EINVAL;

	/*
	 * Already destroyed or not initialized?
	 *
	 * Note that we also check for NULL as on *nix pthread_mutex_t is a struct and
	 * some apps don't use PTHREAD_MUTEX_INITIALIZER assuming that the struct
	 * located in a global data segment is initialized to zeroes by the compiler.
	 * In either case, we return EINVAL as per POSIX.
	 */
	if (!*mutex || *mutex >= PTHREAD_INVALID_MUTEX_INITIALIZER)
		return EINVAL;

	mx = *mutex;

#if defined(USE_FMUTEX)
	/*
	 * This may fail if fmutex was not yet initialized via lock etc, so ignore the
	 * return value.
	 */
	_fmutex_close(&mx->fmutex);
#else
	do {
		rc = DosCloseMutexSem(mx->sem);
		if (rc == ERROR_SEM_BUSY) {
			rc = DosReleaseMutexSem(mx->sem);
			if (rc == ERROR_NOT_OWNER)
				return EBUSY;
		}
	} while (rc == ERROR_SEM_BUSY);
#endif

	free(mx);

	*mutex = PTHREAD_INVALID_MUTEX_INITIALIZER;

	/* Return the completion status: */
	return (0);
}


int
pthread_mutex_lock(pthread_mutex_t * mutex)
{
	APIRET		rc = 0;
	pthread_mutex_t mx;

	if (!mutex || (*mutex && *mutex == PTHREAD_INVALID_MUTEX_INITIALIZER))
		return EINVAL;

	/* Initialize static semaphores created with PTHREAD_MUTEX_INITIALIZER state. */
	if (!*mutex || *mutex >= PTHREAD_ERRORCHECK_MUTEX_INITIALIZER) {
		rc = _pthread_mutex_lazy_init(mutex);
		if (rc)
			return rc;
	}

	mx = *mutex;

#if defined(USE_FMUTEX)
	if (mx->kind == PTHREAD_MUTEX_RECURSIVE && _fmutex_is_owner(&mx->fmutex)) {
		mx->recursive_count++;
		if (mx->recursive_count == 0)
			abort();
	} else {
		rc = _fmutex_request(&mx->fmutex, _FMR_IGNINT);
		if (mx->kind == PTHREAD_MUTEX_RECURSIVE && rc == 0) {
			if (mx->recursive_count)
				abort();
			mx->recursive_count = 1;
		}
	}
#else
	DOS_NI(rc = DosRequestMutexSem(mx->sem,SEM_INDEFINITE_WAIT));
#endif
	if (rc)
		return(__libc_native2errno(rc));

	/* Return the completion status: */
	return (0);
}

int
pthread_mutex_trylock(pthread_mutex_t * mutex)
{
	APIRET		rc = 0;
	pthread_mutex_t mx;

	if (!mutex || (*mutex && *mutex == PTHREAD_INVALID_MUTEX_INITIALIZER))
		return EINVAL;

	/* Initialize static semaphores created with PTHREAD_MUTEX_INITIALIZER state. */
	if (!*mutex || *mutex >= PTHREAD_ERRORCHECK_MUTEX_INITIALIZER) {
		rc = _pthread_mutex_lazy_init(mutex);
		if (rc)
			return rc;
	}

	mx = *mutex;

#if defined(USE_FMUTEX)
	if (mx->kind == PTHREAD_MUTEX_RECURSIVE && _fmutex_is_owner(&mx->fmutex)) {
		mx->recursive_count++;
		if (mx->recursive_count == 0)
			abort();
	} else {
		rc = _fmutex_request(&mx->fmutex, _FMR_IGNINT | _FMR_NOWAIT);
		if (mx->kind == PTHREAD_MUTEX_RECURSIVE && rc == 0) {
			if (mx->recursive_count)
				abort();
			mx->recursive_count = 1;
		}
	}
	if (rc)
		return __libc_native2errno(rc);
#else
	DOS_NI(rc = DosRequestMutexSem(mx->sem,SEM_IMMEDIATE_RETURN));
	if (rc) {
		/*
		 * SEM_IMMEDIATE_RETURN causes ERROR_TIMEOUT if the mutex is already locked
		 * and this code is translated to ETIMEDOUT by __libc_native2errno but POSIX
		 * says this func should return EBUSY (and some apps check that).
		 */
		if (rc == ERROR_TIMEOUT)
			return EBUSY;
		return __libc_native2errno(rc);
  }
#endif

	/* Return the completion status: */
	return (0);
}

int
pthread_mutex_unlock(pthread_mutex_t * mutex)
{
	APIRET		rc = 0;
	pthread_mutex_t mx;

	if (!mutex || (*mutex && *mutex == PTHREAD_INVALID_MUTEX_INITIALIZER))
		return EINVAL;

	/* Initialize static semaphores created with PTHREAD_MUTEX_INITIALIZER state. */
	if (!*mutex || *mutex >= PTHREAD_ERRORCHECK_MUTEX_INITIALIZER) {
		rc = _pthread_mutex_lazy_init(mutex);
		if (rc)
			return rc;
	}

	mx = *mutex;

#if defined(USE_FMUTEX)
	if (mx->kind == PTHREAD_MUTEX_RECURSIVE && _fmutex_is_owner(&mx->fmutex)) {
		if (!mx->recursive_count)
			abort();
		mx->recursive_count--;
		if (!mx->recursive_count)
			rc = _fmutex_release(&mx->fmutex);
	} else {
		rc = _fmutex_release(&mx->fmutex);
	}
#else
	rc = DosReleaseMutexSem(mx->sem);
#endif
	if (rc)
		return __libc_native2errno(rc);

	/* Return the completion status: */
	return (0);
}

int
pthread_mutexattr_init (pthread_mutexattr_t * attr)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      Initializes a mutex attributes object with default
      *      attributes.
      *
      * PARAMETERS
      *      attr
      *              pointer to an instance of pthread_mutexattr_t
      *
      *
      * DESCRIPTION
      *      Initializes a mutex attributes object with default
      *      attributes.
      *
      *      NOTES:
      *              1)      Used to define mutex types
      *
      * RESULTS
      *              0               successfully initialized attr,
      *              ENOMEM          insufficient memory for attr.
      *
      * ------------------------------------------------------
      */
{
  int result = 0;
  pthread_mutexattr_t ma;

  ma = (pthread_mutexattr_t) calloc (1, sizeof (*ma));

  if (ma == NULL)
    {
      result = ENOMEM;
    }
  else
    {
      ma->pshared = PTHREAD_PROCESS_PRIVATE;
      ma->kind = PTHREAD_MUTEX_DEFAULT;
    }

  *attr = ma;

  return (result);
}				/* pthread_mutexattr_init */

int
pthread_mutexattr_destroy (pthread_mutexattr_t * attr)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      Destroys a mutex attributes object. The object can
      *      no longer be used.
      *
      * PARAMETERS
      *      attr
      *              pointer to an instance of pthread_mutexattr_t
      *
      *
      * DESCRIPTION
      *      Destroys a mutex attributes object. The object can
      *      no longer be used.
      *
      *      NOTES:
      *              1)      Does not affect mutexes created using 'attr'
      *
      * RESULTS
      *              0               successfully released attr,
      *              EINVAL          'attr' is invalid.
      *
      * ------------------------------------------------------
      */
{
  int result = 0;

  if (attr == NULL || *attr == NULL)
    {
      result = EINVAL;
    }
  else
    {
      pthread_mutexattr_t ma = *attr;

      *attr = NULL;
      free (ma);
    }

  return (result);
}				/* pthread_mutexattr_destroy */

int
pthread_mutexattr_settype (pthread_mutexattr_t * attr, int kind)
     /*
      * ------------------------------------------------------
      *
      * DOCPUBLIC
      * The pthread_mutexattr_settype() and
      * pthread_mutexattr_gettype() functions  respectively set and
      * get the mutex type  attribute. This attribute is set in  the
      * type parameter to these functions.
      *
      * PARAMETERS
      *      attr
      *              pointer to an instance of pthread_mutexattr_t
      *
      *      type
      *              must be one of:
      *
      *                      PTHREAD_MUTEX_DEFAULT
      *
      *                      PTHREAD_MUTEX_NORMAL
      *
      *                      PTHREAD_MUTEX_ERRORCHECK
      *
      *                      PTHREAD_MUTEX_RECURSIVE
      *
      * DESCRIPTION
      * The pthread_mutexattr_settype() and
      * pthread_mutexattr_gettype() functions  respectively set and
      * get the mutex type  attribute. This attribute is set in  the
      * type  parameter to these functions. The default value of the
      * type  attribute is  PTHREAD_MUTEX_DEFAULT.
      *
      * The type of mutex is contained in the type  attribute of the
      * mutex attributes. Valid mutex types include:
      *
      * PTHREAD_MUTEX_NORMAL
      *          This type of mutex does  not  detect  deadlock.  A
      *          thread  attempting  to  relock  this mutex without
      *          first unlocking it will  deadlock.  Attempting  to
      *          unlock  a  mutex  locked  by  a  different  thread
      *          results  in  undefined  behavior.  Attempting   to
      *          unlock  an  unlocked  mutex  results  in undefined
      *          behavior.
      *
      * PTHREAD_MUTEX_ERRORCHECK
      *          This type of  mutex  provides  error  checking.  A
      *          thread  attempting  to  relock  this mutex without
      *          first unlocking it will return with  an  error.  A
      *          thread  attempting to unlock a mutex which another
      *          thread has locked will return  with  an  error.  A
      *          thread attempting to unlock an unlocked mutex will
      *          return with an error.
      *
      * PTHREAD_MUTEX_DEFAULT
      *          Same as PTHREAD_MUTEX_NORMAL.
      *
      * PTHREAD_MUTEX_RECURSIVE
      *          A thread attempting to relock this  mutex  without
      *          first  unlocking  it  will  succeed in locking the
      *          mutex. The relocking deadlock which can occur with
      *          mutexes of type  PTHREAD_MUTEX_NORMAL cannot occur
      *          with this type of mutex. Multiple  locks  of  this
      *          mutex  require  the  same  number  of  unlocks  to
      *          release  the  mutex  before  another  thread   can
      *          acquire the mutex. A thread attempting to unlock a
      *          mutex which another thread has locked will  return
      *          with  an  error. A thread attempting to  unlock an
      *          unlocked mutex will return  with  an  error.  This
      *          type  of mutex is only supported for mutexes whose
      *          process        shared         attribute         is
      *          PTHREAD_PROCESS_PRIVATE.
      *
      * RESULTS
      *              0               successfully set attribute,
      *              EINVAL          'attr' or 'type' is invalid,
      *
      * ------------------------------------------------------
      */
{
  int result = 0;

  if ((attr != NULL && *attr != NULL))
    {
      switch (kind)
	{
	case PTHREAD_MUTEX_FAST_NP:
	case PTHREAD_MUTEX_RECURSIVE_NP:
	case PTHREAD_MUTEX_ERRORCHECK_NP:
	  (*attr)->kind = kind;
	  break;
	default:
	  result = EINVAL;
	  break;
	}
    }
  else
    {
      result = EINVAL;
    }

  return (result);
}				/* pthread_mutexattr_settype */

int
pthread_mutexattr_gettype (pthread_mutexattr_t * attr, int *kind)
{
  int result = 0;

  if (attr != NULL && *attr != NULL && kind != NULL)
    {
      *kind = (*attr)->kind;
    }
  else
    {
      result = EINVAL;
    }

  return (result);
}

static int forkChild(__LIBC_PFORKHANDLE pForkHandle, __LIBC_FORKOP enmOperation)
{
  if (enmOperation != __LIBC_FORK_OP_FORK_CHILD)
    return 0;

  /* Reset global variables to initial values for the forked child */

  mutex_init_lock = 0;

  return 0;
}

_FORK_CHILD1(0xFFFFFFFE, forkChild);
