#include "worker.h"
#include <pthread.h>

#define THREAD_COUNT 8

int main(void)
{
    int i;
    pthread_t threads[THREAD_COUNT];

    for (i = 0; i < THREAD_COUNT - 1; i++)
        pthread_create(&threads[i], NULL, start_worker, NULL);
    pthread_create(&threads[i], NULL, start_broadcaster, NULL);

    for (i = 0; i < THREAD_COUNT - 1; i++)
        pthread_join(threads[i], NULL);
    pthread_join(threads[i], NULL);
    return 0;
}
