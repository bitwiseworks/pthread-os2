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
** Win32 local thread storage functions emulation
*****************************************************************************/

#define INCL_DOS
#define INCL_EXAPIS
#define INCL_EXAPIS_MAPPINGS
#define _EMX_SOURCE
#include <os2emx.h>

#include <sys/smutex.h>

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

#include "pthread.h"
#include "tls.h"

#include "debug.h"

#define TLS_MINIMUM_AVAILABLE PTHREAD_KEYS_MAX

static _smutex tls_init_lock = 0;

PULONG tls_storage = NULL; /* TLS local storage */
ULONG tls_bits[2]; /* TLS in-use bits (must match TLS_MINIMUM_AVAILABLE) */
pthread_mutex_t tls_mutex = (pthread_mutex_t)NULL; /* TLS mutex for in-use bits */
void (**tls_destructors)(void*); /* TLS destructors */

ULONG TlsAlloc(void)
{
  return TlsAllocEx(NULL);
}

ULONG TlsAllocEx(void (*destructor)(void*))
{
  ULONG index = -1;
  ULONG mask, tibidx;
  int	 i;

  // init memory if not already done
  if (tls_storage == NULL)
    TlsAllocThreadLocalMemory();

  if (tls_mutex == (pthread_mutex_t)NULL) {
    WRN_TID("TlsAllocEx: tls_mutex not initialized!\n");
    return -1;
  }

  pthread_mutex_lock(&tls_mutex);

  if (destructor && !tls_destructors) {
    // Lazily create a TLS destructor array
    tls_destructors = calloc(TLS_MINIMUM_AVAILABLE, sizeof(*tls_destructors));
    if (!tls_destructors) {
      WRN_TID("TlsAllocEx: tls_destructors alloc failed!\n");
      pthread_mutex_unlock(&tls_mutex);
      return -1;
    }
  }

  tibidx = 0;
  if (tls_bits[0] == 0xFFFFFFFF) {
    if (tls_bits[1] == 0xFFFFFFFF) {
      WRN_TID("TlsAllocEx: no more TLS bits available\n");
      pthread_mutex_unlock(&tls_mutex);
      return -1;
    }
    tibidx = 1;
  }
  for(i=0; i<32; i++) {
    mask = (1 << i);
    if ((tls_bits[tibidx] & mask) == 0) {
      tls_bits[tibidx] |= mask;
      index = (tibidx*32) + i;
      break;
    }
  }
  ULONG* tls_array = (ULONG*) *tls_storage;
  if (tls_array)
    tls_array[index] = 0;

  if (destructor)
    tls_destructors[index] = destructor;

  pthread_mutex_unlock(&tls_mutex);

  DBG_TID("TlsAlloc index %lu\n", index);

  return index;
}

BOOL TlsFree(ULONG index)
{
  int	  tlsidx;
  ULONG  mask;

  if (index >= TLS_MINIMUM_AVAILABLE)
    return FALSE;

  pthread_mutex_lock(&tls_mutex);

  tlsidx = 0;
  if (index > 32) {
    tlsidx++;
  }
  mask = (1 << index);
  if (tls_bits[tlsidx] & mask) {
    tls_bits[tlsidx] &= ~mask;
    ULONG* tls_array = (ULONG*) *tls_storage;
    if (tls_array)
      tls_array[index] = 0;
    if (tls_destructors)
      tls_destructors[index] = NULL;
    if (tls_bits[0] == 0 && tls_bits[1] == 0) {
      // the last key was removed, free index & dtor memory to not leak it
      DBG_TID("TlsFree: freeing tls_array %p, tls_destructors %p\n", tls_array, tls_destructors);
      if (tls_array) {
        free(tls_array);
        *tls_storage = 0;
      }
      if (tls_destructors) {
        free(tls_destructors);
        tls_destructors = NULL;
      }
    }
    pthread_mutex_unlock(&tls_mutex);
    return TRUE;
  }

  pthread_mutex_unlock(&tls_mutex);
  return FALSE;
}


PVOID TlsGetValue(ULONG index)
{
  if (index >= TLS_MINIMUM_AVAILABLE)
    return NULL;

  // init memory if not already done
  if (tls_storage == NULL)
    TlsAllocThreadLocalMemory();

  // verify if memory has been allocated for this thread
  if (*tls_storage == (ULONG)NULL) {
    // allocate memory for indexes
    *tls_storage = (ULONG)calloc(TLS_MINIMUM_AVAILABLE, sizeof(int));
    DBG_TID("tls_storage %lx\n", *tls_storage);
  }

  ULONG* tls_array = (ULONG*) *tls_storage;
  return (PVOID) tls_array[index];
}

BOOL TlsSetValue(ULONG index, PVOID val)
{
  // init memory if not already done
  if (tls_storage == NULL)
    TlsAllocThreadLocalMemory();

  // verify if memory has been allocated for this thread
  if (*tls_storage == (ULONG)NULL) {
    // allocate memory for indexes
    *tls_storage = (ULONG)calloc(TLS_MINIMUM_AVAILABLE, sizeof(int));
    DBG_TID("tls_storage %lx\n", *tls_storage);
  }

  if (index >= TLS_MINIMUM_AVAILABLE)
    return FALSE;

  ULONG* tls_array = (ULONG*) *tls_storage;
  DBG_TID("TlsSetValue array %p index %lu -> %p (old)\n", tls_array, index, (void*)tls_array[index]);
  tls_array[index] = (ULONG) val;
  DBG_TID("TlsSetValue array %p index %lu -> %p\n", tls_array, index, val);

  return TRUE;
}


/*
 * Allocate resouces for TLS handling
 */
void TlsAllocThreadLocalMemory(void)
{
  APIRET rc = 0;

  // protect from (rare) simultaneous initializations
  _smutex_request(&tls_init_lock);

  do {
    // check if already called
    if (tls_storage != NULL) {
      WRN_TID("TlsAllocThreadLocalMemory error: init already done!\n");
      break;
    }

    // allocate memory for TLS storage
    rc = DosAllocThreadLocalMemory(1, &tls_storage);
    if (rc) {
      WRN_TID("TlsAllocThreadLocalMemory: DosAllocThreadLocalMemory failed rc=%d\n",(int)rc);
      tls_storage = NULL;
      break;
    }

    // create a mutex
    if ((rc = pthread_mutex_init(&tls_mutex, NULL))) {
      WRN_TID("TlsAllocThreadLocalMemory: tls_mutex init failed rc=%d\n",(int)rc);
      tls_storage = NULL;
      break;
    }
  } while(0);

  _smutex_release(&tls_init_lock);
}

/*
 * Free resouces for TLS handling
 *
 * TODO Call this from DLL_InitTerm when the DLL is unloaded to free global
 * resources (as well as key with index 0 used by phtread_self()).
 */
void TlsFreeThreadLocalMemory(void)
{
  APIRET rc = 0;

  // This must never be called on a thread other than 1
  if (_gettid() != 1) {
    WRN_TID("TlsFreeThreadLocalMemory: called not on thread 1\n");
    return;
  }

  // check if already called
  if (tls_storage == NULL) {
    WRN_TID("TlsFreeThreadLocalMemory: free already done!\n");
    return;
  }

  // free mutex
  pthread_mutex_destroy(&tls_mutex);
  tls_mutex = (pthread_mutex_t)NULL;

  // free memory for TLS storage
  rc = DosFreeThreadLocalMemory(tls_storage);
  if (rc)
    WRN_TID("TlsFreeThreadLocalMemory error: return code = %d\n",(int)rc);

  // reset memory
  tls_storage = NULL;
}

void TlsThreadDestructor(void)
{
  ULONG* tls_array = (ULONG*) *tls_storage;
  if (tls_array) {
    // NOTE: we don't lock the mutex here because:
    // a) adding new keys on other threads will keep this thread's value NULL
    //    (no need to call the destructor)
    // b) removing existing keys should not happen while there are threads that
    //    might use them (like this one); the behavior is undefuned in this case
    // c) destructors are allowed to create/remove keys so we must keep the lock
    //    released
    if (tls_destructors) {
      int pass;
      for (pass = 0; pass < PTHREAD_DESTRUCTOR_ITERATIONS; ++pass) {
        int i;
        for (i = 0; i < TLS_MINIMUM_AVAILABLE; ++i) {
          void *value = (void *)tls_array[i];
          if (value && tls_destructors[i]) {
            tls_array[i] = 0;
            tls_destructors[i](value);
          }
        }
      }
    }
    // free index memory to avoid leaking it when the thread ends
    DBG_TID("TlsThreadDestructor: freeing tls_array %p\n", tls_array);
    free(tls_array);
  }
}
