#include <errno.h>
#include <signal.h>

#include "pthread.h"

int pthread_sigmask(int how, const sigset_t *newmask, sigset_t *oldmask)
{
  return sigprocmask(how, newmask, oldmask) == -1 ? errno : 0;
}

