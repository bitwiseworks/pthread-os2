#define INCL_DOS
#include <os2.h>

#include <malloc.h>
#include <process.h>
#include <strings.h>
#include <sys/timeb.h>

#include "pthread.h"
#include "pthread_private.h"

int pthread_rwlock_init(pthread_rwlock_t *lock,
                                const pthread_rwlockattr_t *attr)
{
	return EINVAL;
}

int pthread_rwlock_destroy(pthread_rwlock_t *lock)
{
	return EINVAL;
}

int pthread_rwlock_tryrdlock(pthread_rwlock_t *lock)
{
	return EINVAL;
}

int pthread_rwlock_trywrlock(pthread_rwlock_t *lock)
{
	return EINVAL;
}

int pthread_rwlock_rdlock(pthread_rwlock_t *lock)
{
	return EINVAL;
}

int pthread_rwlock_timedrdlock(pthread_rwlock_t *lock,
                                       const struct timespec *abstime)
{
	return EINVAL;
}


int pthread_rwlock_wrlock(pthread_rwlock_t *lock)
{
	return EINVAL;
}

int pthread_rwlock_timedwrlock(pthread_rwlock_t *lock,
                                       const struct timespec *abstime)
{
	return EINVAL;
}

int pthread_rwlock_unlock(pthread_rwlock_t *lock)
{
	return EINVAL;
}

int pthread_rwlockattr_init (pthread_rwlockattr_t * attr)
{
	return EINVAL;
}

int pthread_rwlockattr_destroy (pthread_rwlockattr_t * attr)
{
	return EINVAL;
}

int pthread_rwlockattr_getpshared (const pthread_rwlockattr_t * attr,
                                           int *pshared)
{
	return EINVAL;
}

int pthread_rwlockattr_setpshared (pthread_rwlockattr_t * attr,
                                           int pshared)
{
	return EINVAL;
}
