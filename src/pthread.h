
#ifndef __PTHREAD_H__
#define __PTHREAD_H__

#include <errno.h>
#include <time.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Flags for once initialization.
 */
#define PTHREAD_NEEDS_INIT  0
#define PTHREAD_DONE_INIT   1
#define PTHREAD_MUTEX_INITIALIZER -1
#define PTHREAD_COND_INITIALIZER {-1,-1}


typedef int     	pthread_key_t;
typedef uint32_t	pthread_mutex_t;
typedef uint32_t	pthread_t;
typedef struct thread_attr {
    uint32_t 	dwStackSize ;
    uint32_t 	dwCreatingFlag ;
    int 		priority ;
} pthread_attr_t ;

typedef struct { int dummy; } pthread_condattr_t;
typedef struct {
    int		waiting;
    uint32_t	semaphore;
} pthread_cond_t;

typedef int pthread_mutexattr_t;
typedef void	*pthread_addr_t;

#define pthread_handler_decl(A,B) void * A(void *B)
typedef void * (*pthread_handler)(void *);

int pthread_dummy(int ret);

int pthread_create(pthread_t *,pthread_attr_t *,pthread_handler,void *);
pthread_t pthread_self(void);
int pthread_join(  pthread_t thread, pthread_addr_t *status);
#define pthread_detach( A) pthread_dummy(0)

int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr);
int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex);
int pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex,
			   struct timespec const *abstime);
int pthread_cond_signal(pthread_cond_t *cond);
int pthread_cond_broadcast(pthread_cond_t *cond);
int pthread_cond_destroy(pthread_cond_t *cond);
int pthread_attr_init(pthread_attr_t *connect_att);
int pthread_attr_setprio(pthread_attr_t *connect_att,int priority);
int pthread_attr_destroy(pthread_attr_t *connect_att);
void pthread_exit(void *a);	 /* was #define pthread_exit(A) ExitThread(A)*/

void		pthread_cleanup_pop (int execute);
void		pthread_cleanup_push (void (*routine) (void *),
			void *routine_arg);

#define _REENTRANT			1

static int pthread_equal(pthread_t t1,pthread_t t2) { return ( t1 == t2);};

extern int pthread_mutex_init (pthread_mutex_t *, const pthread_mutexattr_t *);
extern int pthread_mutex_lock (pthread_mutex_t *);
extern int pthread_mutex_unlock (pthread_mutex_t *);
extern int pthread_mutex_destroy (pthread_mutex_t *);

#define pthread_key(T,V)  uint32_t V
int pthread_key_create(pthread_key_t *key, void (*destructor)(void*));
int pthread_key_delete(pthread_key_t key);
void *pthread_getspecific(pthread_key_t key);
int pthread_setspecific(pthread_key_t key, const void *value);

void  pthread_setprio( int, int);
//#define my_pthread_setprio(A,B)  pthread_setprio( A, B)
#define pthread_kill(A,B) raise(B)
#define pthread_mutex_trylock(A) pthread_mutex_lock(A)
#define pthread_sigmask(A,B,C) sigprocmask((A),(B),(C))

/* Dummy defines for easier code */
#define pthread_attr_setdetachstate(A,B) pthread_dummy(0)
//#define my_pthread_attr_setprio(A,B) pthread_attr_setprio(A,B)
#define pthread_attr_setscope(A,B)
#define pthread_detach_this_thread()
#define pthread_condattr_init(A) 	0
#define pthread_condattr_destroy(A)	0
#define pthread_mutexattr_init(A) 	0
#define pthread_mutexattr_destroy(A) 	0

typedef int* pthread_once_t;
#define PTHREAD_ONCE_INIT ((pthread_once_t)-1)
int pthread_once(pthread_once_t *once_control, void (*init_routine)(void));

int pthread_attr_getstacksize(const pthread_attr_t *attr, size_t *stacksize);
int pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize); 

void pthread_yield(void);

#define PTHREAD_MUTEX_NORMAL 0
#define PTHREAD_MUTEX_RECURSIVE 1
#define PTHREAD_MUTEX_ERRORCHECK 2
#define PTHREAD_MUTEX_DEFAULT  PTHREAD_MUTEX_NORMAL
#define pthread_mutexattr_settype(A, B) pthread_dummy(0)

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __PTHREAD_H__

