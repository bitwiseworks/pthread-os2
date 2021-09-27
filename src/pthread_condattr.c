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



int pthread_condattr_init(pthread_condattr_t *attr)
{
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *   Initializes a condition variable attributes object
      *   with default attributes.
      *
      * PARAMETERS
      *   attr
      *      pointer to an instance of pthread_condattr_t
      *
      * DESCRIPTION
      *   Initializes a condition variable attributes object
      *   with default attributes.
      *
      *   NOTES:
      *      1)   Use to define condition variable types
      *      2)   It is up to the application to ensure
      *           that it doesn't re-init an attribute
      *           without destroying it first. Otherwise
      *           a memory leak is created.
      *
      * RESULTS
      *      0             successfully initialized attr,
      *      ENOMEM        insufficient memory for attr.
      *
      * ------------------------------------------------------
      */
	pthread_condattr_t attr_result;
	int result = 0;

	attr_result = (pthread_condattr_t) calloc(1, sizeof(*attr_result));
	
	if (attr_result == NULL)
	{
		return ENOMEM;
	}
		
	*attr = attr_result;
	
	return result;
}

int pthread_condattr_destroy(pthread_condattr_t *attr)
{
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *   Destroys a condition variable attributes object
      *   The object can no longer be used.
      *
      * PARAMETERS
      *   attr
      *      pointer to an instance of pthread_condattr_t
      *
      * DESCRIPTION
      *   Destroys a condition variable attributes object
      *   The object can no longer be used.
      *
      *   NOTES:
      *      1)   Does not affect condition variables created
      *           using 'attr'
      *
      * RESULTS
      *      0             successfully released attr,
      *      EINVAL        'attr' is invalid.
      *
      * ------------------------------------------------------
      */
	int result = 0;

	if (attr == NULL || *attr == NULL)
	{
		result = EINVAL;
	}
	else
	{
		(void) free(*attr);
		*attr = NULL;
		result = 0;
	}
		
	return result;
}

int pthread_condattr_getclock(pthread_condattr_t *attr, clockid_t clock_id)
{
	int result = 0;

	if (attr == NULL || *attr == NULL)
	{
		result = EINVAL;
	}

	clock_id = (*attr)->clock_id;

	return result;
}

int pthread_condattr_setclock(pthread_condattr_t *attr, clockid_t clock_id)
{
	int result = 0;

	if (clock_id != CLOCK_MONOTONIC && clock_id != CLOCK_REALTIME)
		result = EINVAL;
		
	(*attr)->clock_id = clock_id;

	return result;
}
