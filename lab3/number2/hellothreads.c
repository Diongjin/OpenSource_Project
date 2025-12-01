
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define NUM_THREADS 3

void *hello_thread(void *arg)
{
    int thread_num = *(int *)arg;
    printf("Thread %d: Hello, World!\n", thread_num);
    return NULL;
}

int main()
{
    pthread_t tid[NUM_THREADS];
    int thread_arg[NUM_THREADS];
    int i, status;

    for (i = 0; i < NUM_THREADS; i++)
    {
        thread_arg[i] = i;
        status = pthread_create(&tid[i], NULL, hello_thread, &thread_arg[i]);
    }

    pthread_exit(NULL);
}

