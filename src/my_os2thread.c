 
#define INCL_DOS
#define INCL_DOSERRORS
#include <os2.h>

#include <stdlib.h>
#include <string.h>
#include <process.h>
#include <types.h>
#include "pthread.h"

#undef pthread_exit

static pthread_mutex_t THR_LOCK_thread;

static pthread_key_t THR_self;

struct pthread_map
{
  ULONG		  pthreadself;
  pthread_handler func;
  void *	  param;
  void *	  rc;
};


static pthread_handler_decl(pthread_start,param)
{
  struct pthread_map* map = (struct pthread_map*) param;
  pthread_handler func = map->func;
  void *func_param = map->param;

  // store data structure pointer in thread self memory
  pthread_setspecific(THR_self, map);

  // wait for beginthread to return
  pthread_mutex_lock(&THR_LOCK_thread);
  pthread_mutex_unlock(&THR_LOCK_thread);
  
  // thread about to start
  map->rc = (*func)(func_param);
  // terminate this thread and free resources
  pthread_exit( map->rc);

  // should never get here!
  return 0;				  /* Safety */
}


int pthread_create(pthread_t *thread_id, pthread_attr_t *attr,
		   pthread_handler func, void *param)
{
  ULONG hThread;
  struct pthread_map *map;

  if (!THR_LOCK_thread)
    pthread_mutex_init(&THR_LOCK_thread,NULL);

  if (!THR_self)
    if (pthread_key_create(&THR_self,NULL)==-1)
      return -1; // fail

  // allocate thread private data structure
  map=(struct pthread_map *)malloc(sizeof(*map));
  if (!map)
    return(-1);

  memset( map, 0, sizeof(struct pthread_map));
  map->func=func;
  map->param=param;

  pthread_mutex_lock(&THR_LOCK_thread);

  hThread=(ULONG)_beginthread((void( *)(void *)) pthread_start, NULL,
			        attr ? (attr->dwStackSize ? attr->dwStackSize : 65535) : 65535, 
				(void*) map);
  map->pthreadself = hThread;
  *thread_id = map;
  pthread_mutex_unlock(&THR_LOCK_thread);

  if (hThread == (ULONG) -1) {
    int error=errno;
    return(error ? error : -1);
  }

  if (attr != NULL)
    pthread_setprio(hThread, attr->priority);

  // ok
  return(0);
}


void pthread_exit(void *a)
{
  // get data structure pointer from thread self memory
  struct pthread_map* map = (struct pthread_map*) pthread_getspecific(THR_self);

  // free resources
  pthread_key_delete(THR_self);
  free( map);

  // let libc terminate this thread
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
  // get data structure pointer from thread self memory
  struct pthread_map* map = (struct pthread_map*) pthread_getspecific(THR_self);
  return (pthread_t) map;
}

/* 
 * pthread_join - Causes the calling thread to wait for the termination of a specified thread
*/
int pthread_join( pthread_t thread, pthread_addr_t *status)
{
  // get data structure
  struct pthread_map* map = (struct pthread_map*) thread;
  APIRET rc;

  // now wait for thread end
  TID tid = map->pthreadself;
  rc = DosWaitThread(&tid, DCWW_WAIT);
  if (rc == ERROR_INVALID_THREADID)
    return ESRCH;
  if (rc != NO_ERROR)
    return EINVAL;
  // thread ended, get value
  if (status != NULL)
    *status = map->rc;

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
  pthread_mutex_t THR_LOCK_once;

  // protect init
  pthread_mutex_lock(&THR_LOCK_once);

  if (*once_control == (pthread_once_t)PTHREAD_ONCE_INIT)
  {
     *once_control = 0;
     init_routine();
  }

  pthread_mutex_unlock(&THR_LOCK_once);
  return 0;
}

void pthread_yield(void)
{
    DosSleep(0);
}

int pthread_setcancelstate(int state, int *oldstate)
{
    return(0);
}

int pthread_setcanceltype(int type, int *oldtype)
{
    return(0);
}

void pthread_testcancel(void)
{
    return;
}

int pthread_atfork(void (*prepare)(void), void (*parent)(void),void (*child)(void))
{
    return(0);
}

