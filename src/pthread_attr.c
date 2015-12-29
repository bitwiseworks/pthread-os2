#define INCL_DOS
#define INCL_EXAPIS
#define INCL_EXAPIS_MAPPINGS
#include <os2emx.h>

#include <malloc.h>
#include <process.h>
#include <strings.h>
#include <sys/timeb.h>

#include "pthread.h"
#include "pthread_private.h"



int pthread_attr_init(pthread_attr_t *attr)
{
	pthread_attr_t attr_result;

	if (attr == NULL)
	{
		/* This is disallowed. */
		return EINVAL;
	}
	
	attr_result = (pthread_attr_t) malloc (sizeof (*attr_result));
	
	if (attr_result == NULL)
	{
		return ENOMEM;
	}
	
	attr_result->dwStackSize	= 0;
	attr_result->dwCreatingFlag	= 0;
	attr_result->priority		= 0;
	attr_result->detachstate	= PTHREAD_CREATE_JOINABLE;
	
	*attr = attr_result;
	
	return 0;
}

int pthread_attr_getstacksize(const pthread_attr_t *attr, size_t *stacksize)
{
	if (*attr)
		*stacksize = (*attr)->dwStackSize;
	else
		return EINVAL;
	return 0;
}

int pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize)
{
	if (*attr)
		(*attr)->dwStackSize = stacksize;
	else
		return EINVAL;
	return 0;
}

int pthread_attr_setprio(pthread_attr_t *attr,int priority)
{
	(*attr)->priority=priority;
	return 0;
}

int pthread_attr_destroy(pthread_attr_t *attr)
{
	free (*attr);
	*attr = NULL;
	return 0;
}

int
pthread_attr_setdetachstate (pthread_attr_t * attr, int detachstate)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      This function specifies whether threads created with
      *      'attr' will run detached.
      *
      * PARAMETERS
      *      attr
      *              pointer to an instance of pthread_attr_t
      *
      *      detachstate
      *              an integer containing one of:
      *
      *              PTHREAD_CREATE_JOINABLE
      *                              Thread ID is valid, must be joined
      *
      *              PTHREAD_CREATE_DETACHED
      *                              Thread ID is invalid, cannot be joined,
      *                              canceled, or modified
      *
      *
      * DESCRIPTION
      *      This function specifies whether threads created with
      *      'attr' will run detached.
      *
      *      NOTES:
      *              1)      You cannot join or cancel detached threads.
      *
      * RESULTS
      *              0               successfully set detach state,
      *              EINVAL          'attr' or 'detachstate' is invalid
      *
      * ------------------------------------------------------
      */
{
  if (attr == NULL)
    {
      return EINVAL;
    }

  if (detachstate != PTHREAD_CREATE_JOINABLE &&
      detachstate != PTHREAD_CREATE_DETACHED)
    {
      return EINVAL;
    }

  (*attr)->detachstate = detachstate;
  return 0;
}


int
pthread_attr_getdetachstate (const pthread_attr_t * attr, int *detachstate)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      This function determines whether threads created with
      *      'attr' will run detached.
      *
      * PARAMETERS
      *      attr
      *              pointer to an instance of pthread_attr_t
      *
      *      detachstate
      *              pointer to an integer into which is returned one
      *              of:
      *
      *              PTHREAD_CREATE_JOINABLE
      *                              Thread ID is valid, must be joined
      *
      *              PTHREAD_CREATE_DETACHED
      *                              Thread ID is invalid, cannot be joined,
      *                              canceled, or modified
      *
      *
      * DESCRIPTION
      *      This function determines whether threads created with
      *      'attr' will run detached.
      *
      *      NOTES:
      *              1)      You cannot join or cancel detached threads.
      *
      * RESULTS
      *              0               successfully retrieved detach state,
      *              EINVAL          'attr' is invalid
      *
      * ------------------------------------------------------
      */
{
  if (attr == NULL || detachstate == NULL)
    {
      *detachstate = PTHREAD_CREATE_DETACHED;
      return EINVAL;
    }

  *detachstate = (*attr)->detachstate;
  return 0;
}
