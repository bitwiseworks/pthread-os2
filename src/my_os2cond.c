/* Copyright (C) Yuri Dario & 2000 MySQL AB
   All the above parties has a full, independent copyright to
   the following code, including the right to use the code in
   any manner without any demands from the other parties.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with this library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
   MA 02111-1307, USA */

/*****************************************************************************
** The following is a simple implementation of posix conditions
*****************************************************************************/

#define INCL_DOS
#define INCL_DOSERRORS
#define INCL_EXAPIS
#define INCL_EXAPIS_MAPPINGS
#include <os2emx.h>

#include <malloc.h>
#include <process.h>
#include <strings.h>
#include <sys/timeb.h>
#include <InnoTekLIBC/errno.h>

//#define DEBUG

#include "pthread.h"
#include "pthread_private.h"


int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr)
{
	APIRET	   rc = 0;
	pthread_cond_t cv = NULL;
	pthread_condattr_t cond_attr = NULL;

	if (cond == NULL)
	{
		return EINVAL;
	}

	cv = (pthread_cond_t) calloc (1, sizeof (*cv));
	if (cv == NULL)
	{
		return ENOMEM;
	}

	cv->attr = (pthread_condattr_t) calloc (1, sizeof (*cond_attr));
	if (cv->attr == NULL)
	{
		return ENOMEM;
	}

	cv->waiting = -1;
	cv->semaphore = -1;

	// right now we only handle the clock attribute
	if (attr == NULL || *attr == NULL)
		cv->attr->clock = 0;
	else
		cv->attr->clock = (*attr)->clock;

	/* Warp3 FP29 or Warp4 FP4 or better required */
	rc = DosCreateEventSem( NULL, (PHEV)&cv->semaphore, 0x0800, 0);
	if (rc) {
		free( cv);
		return __libc_native2errno(rc);
	}

	cv->waiting=0;

#ifdef DEBUG
	printf( "(#%d) pthread_cond_init cond->semaphore %x\n", _gettid(), cv->semaphore);
#endif

	*cond = cv;
	return 0;
}

int pthread_cond_destroy(pthread_cond_t *cond)
{
	APIRET   rc = 0;
	pthread_cond_t cv;

	if (cond == NULL || *cond == NULL)
	{
		return EINVAL;
	}

	cv = *cond;

#ifdef DEBUG
	printf( "(#%d) pthread_cond_destroy cond->semaphore %x\n", _gettid(), cv->semaphore);
#endif

	do {
		rc = DosCloseEventSem(cv->semaphore);
		if (rc == ERROR_SEM_BUSY) DosPostEventSem(cv->semaphore);
	} while (rc == ERROR_SEM_BUSY);

	if (cv->attr)
	{
		(void) free(cv->attr);
		cv->attr = NULL;
	}

	(void) free (cv);

	if (rc)
		return __libc_native2errno(rc);

	return 0;
}


int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
	APIRET	rc = 0;
	int		rval;
	pthread_cond_t cv;

	if (cond == NULL || *cond == NULL)
	{
		return EINVAL;
	}

	// initialize static semaphores created with PTHREAD_COND_INITIALIZER state.
	if (*cond == PTHREAD_COND_INITIALIZER)
		pthread_cond_init( cond, NULL);

	cv = *cond;

	rval = 0;
	cv->waiting++;

#ifdef DEBUG
	printf( "(#%d) pthread_cond_wait cond->semaphore %x, cond->waiting %d\n", _gettid(), cv->semaphore, cv->waiting);
#endif

	if (mutex) pthread_mutex_unlock(mutex);

	DOS_NI(rc = DosWaitEventSem(cv->semaphore,SEM_INDEFINITE_WAIT));
	if (rc != 0)
		rval = __libc_native2errno(rc);

	if (mutex) pthread_mutex_lock(mutex);

	cv->waiting--;
#ifdef DEBUG
	printf( "(#%d) pthread_cond_wait cond->semaphore %x, cond->waiting %d (exit)\n", _gettid(), cv->semaphore, cv->waiting);
#endif

	return rval;
}


int pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex,
			   struct timespec const *abstime)
{
	struct timeb curtime;
	struct timespec ts;
	long timeout;
	APIRET	rc = 0;
	int		rval;
	pthread_cond_t cv;

	if (cond == NULL || *cond == NULL)
	{
		return EINVAL;
	}

	// initialize static semaphores created with PTHREAD_COND_INITIALIZER state.
	if (*cond == PTHREAD_COND_INITIALIZER)
		pthread_cond_init( cond, NULL);

	cv = *cond;

	if (!cv->attr->clock)
	{
		_ftime(&curtime);
		timeout= ((long) (abstime->tv_sec - curtime.time)*1000L +
			(long)((abstime->tv_nsec/1000) - curtime.millitm)/1000L);
	}else{
		clock_gettime (CLOCK_MONOTONIC, &ts);
		timeout= ((long) (abstime->tv_sec - ts.tv_sec)*1000L +
			(long)((abstime->tv_nsec - ts.tv_nsec)/1000)/1000L);
	}

	if (timeout < 0)				/* Some safety */
		timeout = 0L;

	rval = 0;
	cv->waiting++;

#ifdef DEBUG
	printf( "(#%d) pthread_cond_timedwait cond->semaphore %x, cond->waiting %d\n", _gettid(), cv->semaphore, cv->waiting);
#endif

	if (mutex) pthread_mutex_unlock(mutex);

#ifdef DEBUG
	printf( "(#%d) pthread_cond_timedwait waiting timeout %d\n", _gettid(), timeout);
#endif
	DOS_NI(rc = DosWaitEventSem(cv->semaphore, timeout));
#ifdef DEBUG
	printf( "(#%d) pthread_cond_timedwait waiting rc %d\n", _gettid(), rc);
#endif
	if (rc != 0)
		rval = __libc_native2errno(rc);

	if (mutex) pthread_mutex_lock(mutex);

	cv->waiting--;

	return rval;
}


int pthread_cond_signal(pthread_cond_t *cond)
{
	APIRET	rc = 0;
	pthread_cond_t cv;

	if (cond == NULL || *cond == NULL)
	{
		return EINVAL;
	}

	// initialize static semaphores created with PTHREAD_COND_INITIALIZER state.
	if (*cond == PTHREAD_COND_INITIALIZER)
		pthread_cond_init( cond, NULL);

	cv = *cond;

	/* Bring the next thread off the condition queue: */
#ifdef DEBUG
	printf( "(#%d) pthread_cond_signal cond->semaphore %x, cond->waiting %d\n", _gettid(), cv->semaphore, cv->waiting);
#endif
	if (cv->waiting)
		rc = DosPostEventSem(cv->semaphore);
#ifdef DEBUG
	printf( "(#%d) pthread_cond_signal rc %d\n", _gettid(), rc);
#endif
	if (rc && rc != ERROR_ALREADY_POSTED)
		return __libc_native2errno(rc);

	return 0;
}


int pthread_cond_broadcast(pthread_cond_t *cond)
{
	int	    i;
	APIRET	rc = 0;
	pthread_cond_t cv;

	if (cond == NULL || *cond == NULL)
	{
		return EINVAL;
	}

	// initialize static semaphores created with PTHREAD_COND_INITIALIZER state.
	if (*cond == PTHREAD_COND_INITIALIZER)
		pthread_cond_init( cond, NULL);

	cv = *cond;

	/*
	* Enter a loop to bring all threads off the
	* condition queue:
	*/
	i = cv->waiting;
#ifdef DEBUG
	printf( "pthread_cond_broadcast cond->semaphore %x, cond->waiting %d\n", cv->semaphore, cv->waiting);
#endif

	while (i--) rc = DosPostEventSem(cv->semaphore);

	if (rc && rc != ERROR_ALREADY_POSTED)
		return __libc_native2errno(rc);

	return 0 ;
}
