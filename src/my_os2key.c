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

struct pthread_key_pair_t_
{
    pthread_key_t key;
    void (*destructor)(void*);
    struct pthread_key_pair_t_ *next;
};

typedef struct pthread_key_pair_t_ * pthread_key_pair_t;

static pthread_key_pair_t pair_start = NULL;

int pthread_key_create(pthread_key_t *key, void (*destructor)(void*))
{
    pthread_key_pair_t new_pair;

    new_pair = calloc(1, sizeof(*new_pair));
    if (new_pair == NULL) {
        errno = ENOMEM;
        return -1;
    }

    if ((*(ULONG *)key = TlsAlloc()) != -1)
    {
        new_pair->key = *key;
        new_pair->destructor = destructor;
        new_pair->next = pair_start;

        pair_start = new_pair;

        return 0;
    }

    free(new_pair);

    errno = EAGAIN;
    return -1;
}

int pthread_key_delete(pthread_key_t key)
{
    pthread_key_pair_t prev = NULL;
    pthread_key_pair_t pair;

    for (pair = pair_start; pair != NULL; pair = pair->next) {
        if (pair->key == key) {
            if (pair == pair_start)
                pair_start = pair->next;
            else
                prev->next = pair->next;

            free(pair);
            break;
        }

        prev = pair;
    }

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

void pthread_key_destructor(void)
{
    pthread_key_pair_t pair;

    for (pair = pair_start; pair != NULL; pair = pair->next) {
        void *value = pthread_getspecific(pair->key);

        if (value != NULL && pair->destructor != NULL) {
            pthread_setspecific(pair->key, NULL);
            pair->destructor(value);
        }
    }
}
