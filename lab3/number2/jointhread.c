#include <stdio.h>
#include <stdlib.h>     // exit, atoi
#include <pthread.h>
#include <stdint.h>     // intptr_t

void *join_thread(void *arg)
{
    // 그대로 돌려보낼 거면 그냥 pthread_exit(arg); or return arg;
    pthread_exit(arg);
}

int main(int argc, char *argv[])
{
    pthread_t tid;
    int status;
    void *result;

    if (argc < 2) {
        fprintf(stderr, "Usage: jointhread <number>\n");
        exit(1);
    }

    int arg = atoi(argv[1]);

    // int -> 포인터 (정석적인 변환)
    status = pthread_create(&tid, NULL, join_thread, (void *)(intptr_t)arg);
    if (status != 0) {
        fprintf(stderr, "Create thread: %d\n", status);
        exit(1);
    }

    status = pthread_join(tid, &result);
    if (status != 0) {
        fprintf(stderr, "Join thread: %d\n", status);
        exit(1);
    }

    // 포인터 -> int (정석적인 변환)
    int ret = (int)(intptr_t)result;
    printf("thread returned %d\n", ret);

    // 프로세스 리턴 코드는 0~255만 의미 있는 경우가 많아서 보통 0이나 1만 씀
    return ret;
}
