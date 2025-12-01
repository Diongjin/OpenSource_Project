#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>     // sleep()
#include <stdint.h>     // intptr_t

void *cancel_thread(void *arg)
{
    int i, state;
    for (i = 0;; i++)
    {
        /* disables cancelability */
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &state);
        printf("Thread: cancel state disabled\n");
        sleep(1);

        /* restores cancelability */
        pthread_setcancelstate(state, &state);
        printf("Thread: cancel state restored\n");

        if (i % 5 == 0)
            pthread_testcancel();
    }
    return arg;
}

int main(int argc, char *argv[])
{
    pthread_t tid;
    int arg, status;
    void *result;

    if (argc < 2)
    {
        fprintf(stderr, "Usage: cancelthread time(sec)\n");
        exit(1);
    }

    /* 쓰레드 생성 */
    status = pthread_create(&tid, NULL, cancel_thread, NULL);
    if (status != 0)
    {
        fprintf(stderr, "Create thread: %d\n", status);
        exit(1);
    }

    arg = atoi(argv[1]);
    sleep(arg);

    status = pthread_cancel(tid);
    if (status != 0)
    {
        fprintf(stderr, "Cancel thread: %d\n", status);
        exit(1);
    }

    status = pthread_join(tid, &result);
    if (status != 0)
    {
        fprintf(stderr, "Join thread: %d\n", status);
        exit(1);
    }

    // pthread_cancel된 스레드의 리턴값은 (void*) -1 이므로 intptr_t로 변환해야 안전
    return (int)(intptr_t)result;
}
