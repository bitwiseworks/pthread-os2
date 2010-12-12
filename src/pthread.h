
#ifndef __PTHREAD_H__
#define __PTHREAD_H__

#include <errno.h>
#include <time.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Boolean values to make us independent of system includes.
 */
enum {
  PTW32_FALSE = 0,
  PTW32_TRUE = (! PTW32_FALSE)
};

/*
 * Flags for once initialization.
 */
#define PTHREAD_NEEDS_INIT  0
#define PTHREAD_DONE_INIT   1
#define PTHREAD_COND_INITIALIZER {-1,-1}


typedef int     	pthread_key_t;
typedef uint32_t	pthread_mutex_t;
typedef void*		pthread_t;

typedef struct thread_attr {
    uint32_t 	dwStackSize ;
    uint32_t 	dwCreatingFlag ;
    int 		priority ;
    int         detachstate;
} pthread_attr_t ;

typedef struct { int dummy; } pthread_condattr_t;
typedef struct {
    int		waiting;
    uint32_t	semaphore;
} pthread_cond_t;

typedef void	*pthread_addr_t;

#define pthread_handler_decl(A,B) void * A(void *B)
typedef void * (*pthread_handler)(void *);

int pthread_dummy(int ret);

int pthread_create(pthread_t *,pthread_attr_t *,pthread_handler,void *);
pthread_t pthread_self(void);
int pthread_join(  pthread_t thread, pthread_addr_t *status);

int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr);
int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex);
int pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex,
			   struct timespec const *abstime);
int pthread_cond_signal(pthread_cond_t *cond);
int pthread_cond_broadcast(pthread_cond_t *cond);
int pthread_cond_destroy(pthread_cond_t *cond);
void pthread_exit(void *a);	 /* was #define pthread_exit(A) ExitThread(A)*/

void		pthread_cleanup_pop (int execute);
void		pthread_cleanup_push (void (*routine) (void *),
			void *routine_arg);

#define _REENTRANT			1

int pthread_equal(pthread_t t1,pthread_t t2);

#define pthread_key(T,V)  uint32_t V
int pthread_key_create(pthread_key_t *key, void (*destructor)(void*));
int pthread_key_delete(pthread_key_t key);
void *pthread_getspecific(pthread_key_t key);
int pthread_setspecific(pthread_key_t key, const void *value);

void  pthread_setprio( int, int);
int pthread_kill (pthread_t thread, int sig);

#define pthread_sigmask(A,B,C) sigprocmask((A),(B),(C))

/*
 * PThread Attribute Functions
 */
#define pthread_condattr_init(A) 	0
#define pthread_condattr_destroy(A)	0
#define pthread_attr_setscope(A,B)
int pthread_attr_init(pthread_attr_t *connect_att);
int pthread_attr_setprio(pthread_attr_t *connect_att,int priority);
int pthread_attr_destroy(pthread_attr_t *connect_att);
int pthread_attr_getstacksize(const pthread_attr_t *attr, size_t *stacksize);
int pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize); 
int pthread_attr_setdetachstate (pthread_attr_t * attr, int detachstate);
int pthread_attr_getdetachstate (const pthread_attr_t * attr, int *detachstate);

void pthread_yield(void);

/*
 * ====================
 * ====================
 * POSIX Threads
 * ====================
 * ====================
 */

enum {
/*
 * pthread_attr_{get,set}detachstate
 */
  PTHREAD_CREATE_JOINABLE       = 0,  /* Default */
  PTHREAD_CREATE_DETACHED       = 1,

/*
 * pthread_attr_{get,set}inheritsched
 */
  PTHREAD_INHERIT_SCHED         = 0,
  PTHREAD_EXPLICIT_SCHED        = 1,  /* Default */

/*
 * pthread_{get,set}scope
 */
  PTHREAD_SCOPE_PROCESS         = 0,
  PTHREAD_SCOPE_SYSTEM          = 1,  /* Default */

/*
 * pthread_setcancelstate paramters
 */
  PTHREAD_CANCEL_ENABLE         = 0,  /* Default */
  PTHREAD_CANCEL_DISABLE        = 1,

/*
 * pthread_setcanceltype parameters
 */
  PTHREAD_CANCEL_ASYNCHRONOUS   = 0,
  PTHREAD_CANCEL_DEFERRED       = 1,  /* Default */

/*
 * pthread_mutexattr_{get,set}pshared
 * pthread_condattr_{get,set}pshared
 */
  PTHREAD_PROCESS_PRIVATE       = 0,
  PTHREAD_PROCESS_SHARED        = 1,

/*
 * pthread_barrier_wait
 */
  PTHREAD_BARRIER_SERIAL_THREAD = -1
};

/*
 * ====================
 * ====================
 * Cancelation
 * ====================
 * ====================
 */
#define PTHREAD_CANCELED       ((void *) -1)

/*
 * Mutex types.
 */
enum
{
  /* Compatibility with LinuxThreads */
  PTHREAD_MUTEX_FAST_NP,
  PTHREAD_MUTEX_RECURSIVE_NP,
  PTHREAD_MUTEX_ERRORCHECK_NP,
  PTHREAD_MUTEX_TIMED_NP = PTHREAD_MUTEX_FAST_NP,
  PTHREAD_MUTEX_ADAPTIVE_NP = PTHREAD_MUTEX_FAST_NP,
  /* For compatibility with POSIX */
  PTHREAD_MUTEX_NORMAL = PTHREAD_MUTEX_FAST_NP,
  PTHREAD_MUTEX_RECURSIVE = PTHREAD_MUTEX_RECURSIVE_NP,
  PTHREAD_MUTEX_ERRORCHECK = PTHREAD_MUTEX_ERRORCHECK_NP,
  PTHREAD_MUTEX_DEFAULT = PTHREAD_MUTEX_NORMAL
};

/*
 * ====================
 * ====================
 * Object initialisers
 * ====================
 * ====================
 */
#define PTHREAD_MUTEX_INITIALIZER ((pthread_mutex_t) -1)
#define PTHREAD_RECURSIVE_MUTEX_INITIALIZER ((pthread_mutex_t) -2)
#define PTHREAD_ERRORCHECK_MUTEX_INITIALIZER ((pthread_mutex_t) -3)

struct pthread_mutexattr_t_
{
  int pshared;
  int kind;
};
typedef struct pthread_mutexattr_t_ * pthread_mutexattr_t;


int pthread_mutex_init (pthread_mutex_t *, const pthread_mutexattr_t *);
int pthread_mutex_lock (pthread_mutex_t *);
int pthread_mutex_trylock (pthread_mutex_t *);
int pthread_mutex_unlock (pthread_mutex_t *);
int pthread_mutex_destroy (pthread_mutex_t *);


/* CANCEL */

int pthread_setcancelstate(int state, int *oldstate);
int pthread_setcanceltype(int type, int *oldtype);
void pthread_testcancel(void);
int pthread_atfork(void (*prepare)(void), void (*parent)(void),void (*child)(void)); 

/*
 * ====================
 * ====================
 * Once Key
 * ====================
 * ====================
 */
#define PTHREAD_ONCE_INIT       { PTW32_FALSE, PTHREAD_MUTEX_INITIALIZER, 0, 0}

struct pthread_once_t_
{
  unsigned        done;        /* indicates if user function has been executed */
  pthread_mutex_t lock;
  int          reserved1;
  int          reserved2;
};
typedef struct pthread_once_t_ pthread_once_t;
int pthread_once(pthread_once_t *once_control, void (*init_routine)(void));


#ifdef __cplusplus
} // extern "C"
#endif

#endif // __PTHREAD_H__

