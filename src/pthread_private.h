
#ifndef __PTHREAD_PRIVATE_H__
#define __PTHREAD_PRIVATE_H__

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct pthread_t_
{
	ULONG					hThread;
	pthread_handler	func;
	void*						param;
	void*						rc;
	BOOL            			done;
	int             			detachState;
	pthread_mutex_t	cancelLock;	/* Used for async-cancel safety */
	EXCEPTIONREGISTRATIONRECORD exRegRec;
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
};

struct pthread_mutexattr_t_
{
  int pshared;
  int kind;
};


#ifdef __cplusplus
} // extern "C"
#endif

#endif // __PTHREAD_PRIVATE_H__

