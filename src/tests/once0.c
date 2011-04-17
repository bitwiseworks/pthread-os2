#include <stdio.h>

#include <pthread.h>

void init( void )
{
    printf("init called, and should be only once\n");
}

int main( void )
{
    pthread_once_t once = PTHREAD_ONCE_INIT;

    int i;
    
    for( i = 0; i < 10; i++ )
    {
        printf("Call pthread_once() %dth\n", i + 1 );
        pthread_once( &once, init );
    }

    return 0;
}
