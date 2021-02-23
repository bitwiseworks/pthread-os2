#ifndef __PTHREAD_PRIVATE_H__
#define __PTHREAD_PRIVATE_H__

#define USE_FMUTEX

#include <sys/types.h>
#if defined(USE_FMUTEX)
#include <sys/fmutex.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Miscellaneous definitions.
 */

// Suppresses the ERROR_INTERRUPT return value in Dos API calls by retrying the
// operation as long as this code is returned. This is primarily intended to
// avoid unnecessary interrupts of system calls that may happen during POSIX
// signal delivery (e.g. SIGCHLD). Note that the macro cannot be used in
// assignment expressions so call it with the whole assignment as an argument.
#define DOS_NI(expr) while((expr) == ERROR_INTERRUPT)

// set min stack size to 2MB as in pthread_create specs
#define PTHREAD_STACK_DEFAULT                   (2*1024*1024)

struct pthread_t_
{
	ULONG					hThread;
	pthread_handler	func;
	void*						param;
	void*						rc;
	BOOL            			done;
	int             			detachState;
	pthread_mutex_t	cancelLock;	/* Used for async-cancel safety */
};

struct pthread_attr_t_ {
    uint32_t 	dwStackSize ;
    uint32_t 	dwCreatingFlag ;
    int 			priority ;
    int         	detachstate;
};

struct pthread_condattr_t_ {
	int 			dummy;
};

struct pthread_cond_t_ {
    int			waiting;
    uint32_t	semaphore;
};

struct pthread_mutex_t_
{
#if defined(USE_FMUTEX)
  _fmutex fmutex;
  int recursive_count;    /* Number of unlocks a thread needs to perform
                             before the lock is released (recursive
                             mutexes only). */
  int kind;               /* Mutex type. */
#else
	LONG lock_idx;		/* Provides exclusive access to mutex state
		via the Interlocked* mechanism.
		0: unlocked/free.
		1: locked - no other waiters.
		-1: locked - with possible other waiters.
		*/
	int recursive_count;		/* Number of unlocks a thread needs to perform
		before the lock is released (recursive
		mutexes only). */
	int kind;			/* Mutex type. */
	pthread_t ownerThread;
	HMTX sem;
#endif
};

struct pthread_mutexattr_t_
{
  int pshared;
  int kind;
};


//
// Read-write locks
//

struct pthread_rwlockattr_t_
{
  int pshared;
};

struct pthread_rwlock_t_
{
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  //handle_t shared_waiters;
  //handle_t exclusive_waiters;
  int num_shared_waiters;
  int num_exclusive_waiters;
  int num_active;
  pthread_t owner;
};

void TlsThreadDestructor(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __PTHREAD_PRIVATE_H__
