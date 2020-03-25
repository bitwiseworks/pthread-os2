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
#define INCL_EXAPIS
#define INCL_EXAPIS_MAPPINGS
#include <os2emx.h>

#include <stdlib.h>

#include "pthread.h"
#include "tls.h"

int pthread_key_create(pthread_key_t *key, void (*destructor)(void*))
{
    if ((*(ULONG *)key = TlsAllocEx(destructor)) != -1)
        return 0;

    errno = EAGAIN;
    return -1;
}

int pthread_key_delete(pthread_key_t key)
{
    if (TlsFree((ULONG)key))
        return 0;
    errno = EINVAL;
    return -1;
}

void *pthread_getspecific(pthread_key_t key)
{
    return TlsGetValue((ULONG)key);
}

int pthread_setspecific(pthread_key_t key, const void *value)
{
    if (TlsSetValue((ULONG)key, (PVOID)value))
        return 0;
    errno = EINVAL;
    return -1;
}
