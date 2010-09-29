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
** Simulation of posix threads calls for WIN95 and NT
*****************************************************************************/

/* SAFE_MUTEX will not work until the thread structure is up to date */
#undef SAFE_MUTEX
 
#define INCL_DOS
#include <os2.h>

#include <stdlib.h>
#include <string.h>
#include <process.h>
#include <types.h>
#include "pthread.h"

#undef pthread_exit

static pthread_mutex_t THR_LOCK_thread;

struct pthread_map
{
  ULONG		  pthreadself;
  pthread_handler func;
  void *	  param;
};

/*
** We have tried to use '_beginthreadex' instead of '_beginthread' here
** but in this case the program leaks about 512 characters for each
** created thread !
** As we want to save the created thread handler for other threads to
** use and to be returned by pthread_self() (instead of the Win32 pseudo
** handler), we have to go trough pthread_start() to catch the returned handler
** in the new thread.
*/

static pthread_handler_decl(pthread_start,param)
{
  pthread_handler func=((struct pthread_map *) param)->func;
  void *func_param=((struct pthread_map *) param)->param;
  //my_thread_init();			/* Will always succeed in windows */
  pthread_mutex_lock(&THR_LOCK_thread);   /* Wait for beginthread to return */
  //win_pthread_self=((struct pthread_map *) param)->pthreadself;
  pthread_mutex_unlock(&THR_LOCK_thread);
  free((char*) param);			  /* Free param from create */
  pthread_exit((void*) (*func)(func_param));
  return 0;				  /* Safety */
}


int pthread_create(pthread_t *thread_id, pthread_attr_t *attr,
		   pthread_handler func, void *param)
{
  ULONG hThread;
  struct pthread_map *map;
  //DBUG_ENTER("pthread_create");

  if (!THR_LOCK_thread)
    pthread_mutex_init(&THR_LOCK_thread,NULL);

  if (!(map=(struct pthread_map *)malloc(sizeof(*map))))
    return(-1);

  map->func=func;
  map->param=param;
  pthread_mutex_lock(&THR_LOCK_thread);
#ifdef __BORLANDC__
  hThread=(ULONG)_beginthread((void(_USERENTRY *)(void *)) pthread_start,
			       attr->dwStackSize ? attr->dwStackSize :
			       65535, (void*) map);
#elif defined( OS2) || defined(__EMX__)
  hThread=(ULONG)_beginthread((void( *)(void *)) pthread_start, NULL,
			        attr ? (attr->dwStackSize ? attr->dwStackSize : 65535) : 65535, 
				(void*) map);
#else
  hThread=(ULONG)_beginthread((void( __cdecl *)(void *)) pthread_start,
			       attr->dwStackSize ? attr->dwStackSize :
			       65535, (void*) map);
#endif
  //DBUG_PRINT("info", ("hThread=%lu",(long) hThread));
  *thread_id=map->pthreadself=hThread;
  pthread_mutex_unlock(&THR_LOCK_thread);

  if (hThread == (ULONG) -1)
  {
    int error=errno;
    //DBUG_PRINT("error",("Can't create thread to handle request (error %d)",error));
    return(error ? error : -1);
  }
#if defined( OS2) || defined(__EMX__)
  if (attr != NULL)
    pthread_setprio(hThread, attr->priority);
#else
  VOID(SetThreadPriority(hThread, attr->priority)) ;
#endif
  return(0);
}


void pthread_exit(void *a)
{
  _endthread();
}

/* This is neaded to get the macro pthread_setspecific to work */

int win_pthread_setspecific(void *a,void *b,uint length)
{
  memcpy(a,b,length);
  return 0;
}

void pthread_setprio( int a, int b)
{
   DosSetPriority(PRTYS_THREAD,PRTYC_NOCHANGE, b, a);   
}

int pthread_dummy(int ret)
{
  return ret;
}

/* 
 * pthread_self - Obtains the identifier of the current thread
*/
pthread_t pthread_self(void)
{
  return _gettid();
}

/* 
 * pthread_join - Causes the calling thread to wait for the termination of a specified thread
*/
int pthread_join( pthread_t thread, pthread_addr_t *status)
{
  TID tid = thread;
  DosWaitThread(&tid, DCWW_WAIT);
  return 0;
}

void pthread_cleanup_push(void (*routine) (void *), void *routine_arg)
{
}

void pthread_cleanup_pop(int execute)
{
}

int pthread_once(pthread_once_t *once_control, void (*init_routine)(void))
{
  DosEnterCritSec();
  if (*once_control == (pthread_once_t)PTHREAD_ONCE_INIT)
  {
     *once_control = 0;
     init_routine();
  }
  DosExitCritSec();
  return 0;
}

void pthread_yield(void)
{
    DosSleep(0);
}
