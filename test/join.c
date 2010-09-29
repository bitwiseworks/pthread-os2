#include <pthread.h>
int check_me = 0;
void* func(void* data) {
puts("init");
check_me = 42; 
return &check_me;
}
int main()
 { pthread_t t;
   void *ret;
puts("_create");
   pthread_create (&t, 0, func, 0);
puts("_join");
   pthread_join (t, &ret);
printf("check_me %d\n",check_me);
printf("check_me %x\n",&check_me);
printf("ret %x\n", ret);
   return (check_me != 42 || ret != &check_me);
}

