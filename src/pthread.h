#ifndef __PTHREAD_H__
#define __PTHREAD_H__

#include <errno.h>
#include <time.h>
#include <signal.h>
#include <sys/param.h> /* for PAGE_SIZE */

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

typedef struct pthread_t_ * pthread_t;
typedef struct pthread_attr_t_ * pthread_attr_t;
typedef struct pthread_once_t_ pthread_once_t;
typedef int pthread_key_t; /* be compatible with BSD */
typedef struct pthread_mutex_t_ * pthread_mutex_t;
typedef struct pthread_mutexattr_t_ * pthread_mutexattr_t;
typedef struct pthread_cond_t_ * pthread_cond_t;
typedef struct pthread_condattr_t_ * pthread_condattr_t;

//
// Read-write locks
//

typedef struct pthread_rwlockattr_t_ * pthread_rwlockattr_t;
typedef struct pthread_rwlock_t_ * pthread_rwlock_t;

#define PTHREAD_RWLOCK_INITIALIZER ((pthread_rwlock_t) -1)

//
//
//

struct pthread_once_t_
{
  unsigned        		done;        /* indicates if user function has been executed */
  pthread_mutex_t lock;
  int          			reserved1;
  int          			reserved2;
};

/*
 * Flags for once initialization.
 */
#define PTHREAD_NEEDS_INIT  0
#define PTHREAD_DONE_INIT   1

#define PTHREAD_COND_INITIALIZER ((pthread_cond_t) -1)

#define pthread_handler_decl(A,B) void * A(void *B)
typedef void * (*pthread_handler)(void *);

int pthread_dummy(int ret);

int pthread_create(pthread_t *,const pthread_attr_t *,pthread_handler,void *);
pthread_t pthread_self(void);
int pthread_join(  pthread_t thread, void **status);

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
int pthread_detach (pthread_t thread);
int pthread_kill (pthread_t thread, int sig);

int pthread_sigmask(int, const sigset_t *, sigset_t *);

/*
 * PThread Attribute Functions
 */
int pthread_condattr_init(pthread_condattr_t *attr);
int pthread_condattr_destroy(pthread_condattr_t *attr);
int pthread_condattr_setclock(pthread_condattr_t *attr, clockid_t clock_id);
int pthread_condattr_getclock(pthread_condattr_t *attr, clockid_t clock_id);
#define pthread_attr_setscope(A,B)
int pthread_attr_init(pthread_attr_t *connect_att);
int pthread_attr_setprio(pthread_attr_t *connect_att,int priority);
int pthread_attr_destroy(pthread_attr_t *connect_att);
int pthread_attr_getstacksize(const pthread_attr_t *attr, size_t *stacksize);
int pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize); 
int pthread_attr_setdetachstate (pthread_attr_t * attr, int detachstate);
int pthread_attr_getdetachstate (const pthread_attr_t * attr, int *detachstate);

int pthread_yield(void);

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
 * pthread_attr_setstacksize
 */
  PTHREAD_STACK_MIN             = PAGE_SIZE,

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
 * pthread_key constants
 */
  PTHREAD_DESTRUCTOR_ITERATIONS = 4,
  PTHREAD_KEYS_MAX              = 64,

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

int pthread_mutex_init (pthread_mutex_t *, const pthread_mutexattr_t *);
int pthread_mutex_lock (pthread_mutex_t *);
int pthread_mutex_trylock (pthread_mutex_t *);
int pthread_mutex_unlock (pthread_mutex_t *);
int pthread_mutex_destroy (pthread_mutex_t *);

int pthread_mutexattr_init (pthread_mutexattr_t * attr);
int pthread_mutexattr_destroy (pthread_mutexattr_t * attr);
int pthread_mutexattr_settype (pthread_mutexattr_t * attr, int kind);
int pthread_mutexattr_gettype (pthread_mutexattr_t * attr, int *kind);

/* CANCEL */

int pthread_setcancelstate(int state, int *oldstate);
int pthread_setcanceltype(int type, int *oldtype);
void pthread_testcancel(void);
int pthread_cancel(pthread_t thread);
int pthread_atfork(void (*prepare)(void), void (*parent)(void),void (*child)(void));

/*
 * Read-Write Lock Functions
 */
int pthread_rwlock_init(pthread_rwlock_t *lock,
                                const pthread_rwlockattr_t *attr);

int pthread_rwlock_destroy(pthread_rwlock_t *lock);

int pthread_rwlock_tryrdlock(pthread_rwlock_t *);

int pthread_rwlock_trywrlock(pthread_rwlock_t *);

int pthread_rwlock_rdlock(pthread_rwlock_t *lock);

int pthread_rwlock_timedrdlock(pthread_rwlock_t *lock,
                                       const struct timespec *abstime);

int pthread_rwlock_wrlock(pthread_rwlock_t *lock);

int pthread_rwlock_timedwrlock(pthread_rwlock_t *lock,
                                       const struct timespec *abstime);

int pthread_rwlock_unlock(pthread_rwlock_t *lock);

int pthread_rwlockattr_init (pthread_rwlockattr_t * attr);

int pthread_rwlockattr_destroy (pthread_rwlockattr_t * attr);

int pthread_rwlockattr_getpshared (const pthread_rwlockattr_t * attr,
                                           int *pshared);

int pthread_rwlockattr_setpshared (pthread_rwlockattr_t * attr,
                                           int pshared);

/*
 * ====================
 * ====================
 * Once Key
 * ====================
 * ====================
 */
#define PTHREAD_ONCE_INIT       { PTW32_FALSE, PTHREAD_MUTEX_INITIALIZER, 0, 0}

int pthread_once(pthread_once_t *once_control, void (*init_routine)(void));

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __PTHREAD_H__
