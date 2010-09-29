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

#define INCL_DOS
#include <os2.h>

#include <stdio.h>
#include <stddef.h>

#include "pthread.h"

ULONG	 TlsAlloc( void);
BOOL	 TlsFree( ULONG);
PVOID	 TlsGetValue( ULONG);
BOOL	 TlsSetValue( ULONG, PVOID);
void      TlsAllocThreadLocalMemory( void);
void      TlsFreeThreadLocalMemory( void);

#define TLS_MINIMUM_AVAILABLE	64

PULONG		tls_storage = NULL;	/* TLS local storage */
ULONG		 tls_bits[2];	/* TLS in-use bits   */
pthread_mutex_t tls_mutex = NULL;	/* TLS mutex for in-use bits */

ULONG	 TlsAlloc( void)
{
  ULONG index = -1;
  ULONG mask, tibidx;
  int	 i;

   // init memory if not already done
  if (tls_storage == NULL)
      TlsAllocThreadLocalMemory();

   if (tls_mutex == NULL) {
      fprintf( stderr, "TlsAlloc: tls_mutex not initialized!");
      return -1;
  }

  pthread_mutex_lock( &tls_mutex);

  tibidx = 0;
   if (tls_bits[0] == 0xFFFFFFFF) {
      if (tls_bits[1] == 0xFFFFFFFF) {
            fprintf( stderr, "TlsAlloc: tid#%d, no more TLS bits available\n", _threadid);
      pthread_mutex_unlock( &tls_mutex);
      return -1;
    }
    tibidx = 1;
  }
   for( i=0; i<32; i++) {
    mask = (1 << i);
      if ((tls_bits[ tibidx] & mask) == 0) {
      tls_bits[ tibidx] |= mask;
      index = (tibidx*32) + i;
      break;
    }
  }
  tls_storage[index] = 0;

  pthread_mutex_unlock( &tls_mutex);

   //fprintf( stderr, "tid#%d, TlsAlloc index %d\n", _threadid, index);

  return index;
}

BOOL TlsFree( ULONG index)
{
  int	  tlsidx;
  ULONG  mask;

  if (index >= TLS_MINIMUM_AVAILABLE)
      return FALSE;

  pthread_mutex_lock( &tls_mutex);

  tlsidx = 0;
   if (index > 32) {
    tlsidx++;
   }
  mask = (1 << index);
   if (tls_bits[ tlsidx] & mask) {
    tls_bits[tlsidx] &= ~mask;
    tls_storage[index] = 0;
    pthread_mutex_unlock( &tls_mutex);
    return TRUE;
  }

  pthread_mutex_unlock( &tls_mutex);
  return FALSE;
}


PVOID TlsGetValue( ULONG index)
{
  if (index >= TLS_MINIMUM_AVAILABLE)
    return NULL;

   // verify if memory has been allocated for this thread
   if (*tls_storage == NULL) {
      // allocate memory for indexes
    *tls_storage = (ULONG)calloc( TLS_MINIMUM_AVAILABLE, sizeof(int));
      //fprintf( stderr, "tid#%d, tls_storage %x\n", _threadid, *tls_storage);
  }

  ULONG* tls_array = (ULONG*) *tls_storage;
   return (PVOID) tls_array[ index];
}

BOOL TlsSetValue( ULONG index, PVOID val)
{

   // verify if memory has been allocated for this thread
   if (*tls_storage == NULL) {
      // allocate memory for indexes
    *tls_storage = (ULONG)calloc( TLS_MINIMUM_AVAILABLE, sizeof(int));
      //fprintf( stderr, "tid#%d, tls_storage %x\n", _threadid, *tls_storage);
  }

  if (index >= TLS_MINIMUM_AVAILABLE)
    return FALSE;

  ULONG* tls_array = (ULONG*) *tls_storage;
   //fprintf( stderr, "tid#%d, TlsSetValue array %08x index %d -> %08x (old)\n", _threadid, tls_array, index, tls_array[ index]);
  tls_array[ index] = (ULONG) val;
   //fprintf( stderr, "tid#%d, TlsSetValue array %08x index %d -> %08x\n", _threadid, tls_array, index, val);

  return TRUE;
}


/*
 * Allocate resouces for TLS handling
 * Called by mysql.dll init code
*/
void      TlsAllocThreadLocalMemory( void)
{
   APIRET rc = 0;

   // check if already called
   if (tls_storage != NULL) {
      fprintf( stderr, "TlsAllocThreadLocalMemory error: init already done!\n");
      return;
   }

   // allocate memory for TLS storage
   rc = DosAllocThreadLocalMemory( 1, &tls_storage);
   if (rc) {
      fprintf( stderr, "TlsAllocThreadLocalMemory: DosAllocThreadLocalMemory failed rc=%u\n", rc);
      tls_storage = NULL;
      return;
   }

   // create a mutex
   if (rc = pthread_mutex_init( &tls_mutex, NULL)) {
      fprintf( stderr, "TlsAllocThreadLocalMemory: tls_mutex init failed rc=%u\n", rc);
      tls_storage = NULL;
      return;
   }

}

/*
 * Free resouces for TLS handling
 * Called by mysql.dll init code
*/
void      TlsFreeThreadLocalMemory( void)
{
   APIRET rc = 0;

   // check if already called
   if (tls_storage == NULL) {
      fprintf( stderr, "TlsFreeThreadLocalMemory error: free already done!\n");
      return;
   }

   // free mutex
   pthread_mutex_destroy( &tls_mutex);
   tls_mutex = NULL;

   // free memory for TLS storage
   rc = DosFreeThreadLocalMemory( tls_storage);
   if (rc)
      fprintf( stderr, "DosFreeThreadLocalMemory error: return code = %u\n", rc);

   // reset memory
   tls_storage = NULL;
}
