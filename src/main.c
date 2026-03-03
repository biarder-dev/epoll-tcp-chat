#include "worker.h"
#include <pthread.h>

#define THREAD_COUNT 8

int main(void)
{
    int i;
    pthread_t threads[THREAD_COUNT];

    for (i = 0; i < THREAD_COUNT; i++)
        pthread_create(&threads[i], NULL, start_worker, NULL);
    for (i = 0; i < THREAD_COUNT; i++)
        pthread_join(threads[i], NULL);
    return 0;
}
