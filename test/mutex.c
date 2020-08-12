#include <pthread.h>

static pthread_mutex_t foo_mutex;

int main()
{
   pthread_mutex_lock(&foo_mutex);
   /* */
   pthread_mutex_unlock(&foo_mutex);

   return 0;
}

