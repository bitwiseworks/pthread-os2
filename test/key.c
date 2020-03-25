#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#include <pthread.h>

pthread_key_t key = -1;

void key_dtor(void *val)
{
  printf ("tid %d: key_dtor: val: %p\n", _gettid(), val);
}

void *thread(void *unused)
{
  _sleep2(0);

  if (pthread_setspecific(key, (void *)0x1000 + _gettid()) == -1)
  {
    perror ("pthread_setspecific");
    exit (1);
  }

  printf ("tid %d: key val: %p\n", _gettid(), pthread_getspecific(key));

  sleep(1);

  return NULL;
}

int main()
{
  int i;

  if (pthread_key_create(&key, key_dtor) == -1)
  {
    perror ("pthread_key_create");
    return 1;
  }
  printf ("key: %d\n", key);

  printf ("key val: %p\n", pthread_getspecific(key));
  if (pthread_setspecific(key, (void *)0x1001) == -1)
  {
    perror ("pthread_setspecific");
    return 1;
  }
  printf ("key val: %p\n", pthread_getspecific(key));

  for (i = 0; i < 10; ++i)
  {
    if (i % 2)
    {
      pthread_t thr;
      if (pthread_create(&thr, NULL, thread, NULL) == -1)
      {
        perror ("pthread_create");
        return 1;
      }
    }
    else
    {
      if (_beginthread((void (*)(void *))thread, NULL, 0, NULL) == -1)
      {
        perror ("_beginthread");
        return 1;
      }
    }
  }

  sleep(2);

  if (pthread_key_delete(key) == -1)
    perror ("pthread_key_delete");

  return 0;
}
