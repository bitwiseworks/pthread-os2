//
// rwlock.c
//
// POSIX read-write locks
//
// Copyright (C) 2002 Michael Ringgaard. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 
// 1. Redistributions of source code must retain the above copyright 
//    notice, this list of conditions and the following disclaimer.  
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.  
// 3. Neither the name of the project nor the names of its contributors
//    may be used to endorse or promote products derived from this software
//    without specific prior written permission. 
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
// OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
// OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF 
// SUCH DAMAGE.
// 

#define INCL_DOS
#define INCL_EXAPIS
#define INCL_EXAPIS_MAPPINGS
#include <os2emx.h>

#include <malloc.h>

#include "pthread.h"
#include "pthread_private.h"

#define NOHANDLE 0

int pthread_rwlockattr_init(pthread_rwlockattr_t *_attr)
{
  pthread_rwlockattr_t attr = *_attr;
  if (!attr) return EINVAL;
  attr->pshared = PTHREAD_PROCESS_PRIVATE;
  return 0;
}

int pthread_rwlockattr_destroy(pthread_rwlockattr_t *attr)
{
  return 0;
}

int pthread_rwlockattr_getpshared(const pthread_rwlockattr_t *_attr, int *pshared)
{
  pthread_rwlockattr_t attr = *_attr;
  if (!attr || !pshared) return EINVAL;
  *pshared = attr->pshared;
  return 0;
}

int pthread_rwlockattr_setpshared(pthread_rwlockattr_t *_attr, int pshared)
{
  pthread_rwlockattr_t attr = *_attr;
  if (!attr) return EINVAL;
  if (pshared != PTHREAD_PROCESS_PRIVATE && pshared != PTHREAD_PROCESS_SHARED) return EINVAL;
  attr->pshared = pshared;
  return 0;
}

int pthread_rwlock_init(pthread_rwlock_t *_lock, const pthread_rwlockattr_t *_attr)
{
  int rc;
  pthread_rwlock_t lock = NULL;
  pthread_rwlockattr_t attr;

  if (!_lock) return EINVAL;
  if (_attr) {
    attr = *_attr;
    if (attr && attr->pshared == PTHREAD_PROCESS_SHARED) return ENOSYS;
  }

  lock = (pthread_rwlock_t) calloc(1, sizeof (*lock));
  if (lock == NULL) {
      return ENOMEM;
  }

  lock->mutex = 0;
  lock->cond = 0;
  lock->num_active = 0;
  //lock->exclusive_waiters = 0;
  lock->num_shared_waiters = 0;
  lock->owner = NOHANDLE;
  
  rc = pthread_mutex_init(&lock->mutex, NULL);
  if (rc != 0) return rc;

  rc = pthread_cond_init(&lock->cond, NULL);
  if (rc != 0 ) {
      pthread_mutex_destroy(&lock->mutex);

      return rc;
  }

  //lock->exclusive_waiters = mksem(0);
  //if (lock->exclusive_waiters < 0) return errno;

  //lock->shared_waiters = mksem(0);
  //if (lock->shared_waiters < 0) return errno;

  *_lock = lock;

  return 0;
}

int pthread_rwlock_destroy(pthread_rwlock_t *_lock)
{
  pthread_rwlock_t lock = *_lock;
  if (!lock) return EINVAL;
  pthread_cond_destroy(&lock->cond);
  pthread_mutex_destroy(&lock->mutex);
  //if (close(lock->exclusive_waiters) < 0) return errno;
  //if (close(lock->shared_waiters) < 0) return errno;

  free(lock);
  *_lock = PTHREAD_RWLOCK_INITIALIZER;

  return 0;
}

int pthread_rwlock_tryrdlock(pthread_rwlock_t *_lock)
{
  int rc = 0;
  pthread_rwlock_t lock = NULL;

  if (*_lock == PTHREAD_RWLOCK_INITIALIZER) {
      int result = pthread_rwlock_init(_lock, NULL);
      if (result != 0) {
         return result;
      }
  }
  lock = *_lock;

  pthread_mutex_lock(&lock->mutex);
  if (lock->num_active >= 0)
  {
    // Lock is free or acquired by other readers
    lock->num_active++;
  }
  else
    rc = EBUSY;

  pthread_mutex_unlock(&lock->mutex);
  return rc;
}

int pthread_rwlock_trywrlock(pthread_rwlock_t *_lock)
{
  int rc = 0;
  pthread_rwlock_t lock = NULL;

  if (*_lock == PTHREAD_RWLOCK_INITIALIZER) {
      int result = pthread_rwlock_init(_lock, NULL);
      if (result != 0) {
         return result;
      }
  }
  lock = *_lock;

  pthread_mutex_lock(&lock->mutex);
  if (lock->num_active == 0)
  {
    // Lock is free
    lock->num_active = -1;
    lock->owner = pthread_self();
  }
  else
    rc = EBUSY;

  pthread_mutex_unlock(&lock->mutex);
  return rc;
}

int pthread_rwlock_rdlock(pthread_rwlock_t *lock)
{
  return pthread_rwlock_timedrdlock(lock, NULL);
}

int pthread_rwlock_timedrdlock(pthread_rwlock_t *_lock, const struct timespec *abstime)
{
  int rc = 0;
  pthread_rwlock_t lock = NULL;

  if (*_lock == PTHREAD_RWLOCK_INITIALIZER) {
    int result = pthread_rwlock_init(_lock, NULL);
    if (result != 0) {
       return result;
    }
  }
  lock = *_lock;

  pthread_mutex_lock(&lock->mutex);

  while (1)
  {
    if (lock->num_active < 0)
    {
      if (pthread_equal(lock->owner, pthread_self()))
      {
        rc = EDEADLK;
        break;
      }
      //lock->num_shared_waiters++;
      //pthread_mutex_unlock(&lock->mutex);
      //if (waitone(lock->shared_waiters, __abstime2timeout(abstime)) < 0) return errno;
      //pthread_mutex_lock(&lock->mutex);
    }
    else
    {
      lock->num_active++;
      break;
    }

    pthread_cond_wait(&lock->cond, &lock->mutex);
  }

  pthread_mutex_unlock(&lock->mutex);
  return rc;
}

int pthread_rwlock_wrlock(pthread_rwlock_t *lock)
{
  return pthread_rwlock_timedwrlock(lock, NULL);
}

int pthread_rwlock_timedwrlock(pthread_rwlock_t *_lock, const struct timespec *abstime)
{
  int rc = 0;
  pthread_rwlock_t lock = NULL;

  if (*_lock == PTHREAD_RWLOCK_INITIALIZER) {
    int result = pthread_rwlock_init(_lock, NULL);
    if (result != 0) {
       return result;
    }
  }
  lock = *_lock;

  pthread_mutex_lock(&lock->mutex);

  while (1)
  {
    if (lock->num_active == 0)
    {
      // Lock is free
      lock->num_active = -1;
      lock->owner = pthread_self();
      break;
    }
    else if (lock->num_active < 0)
    {
      // Exclusive lock in progress
      if (pthread_equal(lock->owner, pthread_self()))
      {
        rc = EDEADLK;
        break;
      }
    }

    // Wait for lock to be released
    //lock->exclusive_waiters++;
    //pthread_mutex_unlock(&lock->mutex);
    //if (waitone(lock->exclusive_waiters, __abstime2timeout(abstime)) < 0) return errno;
    //pthread_mutex_lock(&lock->mutex);

    pthread_cond_wait(&lock->cond, &lock->mutex);
  }

  pthread_mutex_unlock(&lock->mutex);
  return rc;
}

int pthread_rwlock_unlock(pthread_rwlock_t *_lock)
{
  pthread_rwlock_t lock = NULL;

  if (*_lock == PTHREAD_RWLOCK_INITIALIZER) {
    int result = pthread_rwlock_init(_lock, NULL);
    if (result != 0) {
       return result;
    }
  }
  lock = *_lock;

  pthread_mutex_lock(&lock->mutex);

  if (lock->num_active > 0)
  {
    // Have one or more readers
    if (--lock->num_active == 0)
    {
      if (lock->num_exclusive_waiters > 0)
      {
        lock->num_exclusive_waiters--;
        //semrel(lock->exclusive_waiters, 1);
      }
    }
  }
  else if (lock->num_active < 0)
  {
    // Have a writer, possibly recursive
    if (++lock->num_active == 0)
    {
      lock->owner = NOHANDLE;
      if (lock->num_exclusive_waiters > 0)
      {
        lock->num_exclusive_waiters--;
        //semrel(lock->exclusive_waiters, 1);
      }
      else if (lock->num_shared_waiters > 0)
      {
        int num = lock->num_shared_waiters;
        lock->num_active = num;
        lock->num_shared_waiters = 0;
        //semrel(lock->shared_waiters, num);
      }
    }
  }

  pthread_cond_broadcast(&lock->cond);
  pthread_mutex_unlock(&lock->mutex);
  return 0;
}
