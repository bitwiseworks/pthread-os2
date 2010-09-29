/* Copyright (C) 2000 MySQL AB

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */

/*****************************************************************************
** Pthread local thread storage functions
*****************************************************************************/
 
#define INCL_DOS
#include <os2.h>

#include "pthread.h"
#include "tls.h"

int pthread_key_create(pthread_key_t *key, void (*destructor)(void*))
{
    return (*key=TlsAlloc())==0xFFFFFFFF;
}

int pthread_key_delete(pthread_key_t key)
{
    return TlsFree(key);
}

void *pthread_getspecific(pthread_key_t key)
{
    return TlsGetValue(key);
}

int pthread_setspecific(pthread_key_t key, const void *value)
{
    return !TlsSetValue( key, (PVOID)value);
}

