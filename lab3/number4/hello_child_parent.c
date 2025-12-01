/* hello_parent_child.c */
/* 부모/자식 쓰레드가 번갈아가며 hello 메시지 출력 – 이진 플래그 + 조건변수 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#define NUM_ITER 2   // 한 번씩 번갈아가는 횟수

// turn == 0 : 부모
// turn == 1 : 자식
int turn = 0;  // 부모 먼저

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  cond  = PTHREAD_COND_INITIALIZER;

/* 자식 쓰레드 함수 */
void *child_thread(void *arg)
{
    for (int i = 0; i < NUM_ITER; i++) {
        pthread_mutex_lock(&mutex);

        // 내 차례( turn == 1 ) 가 아닐 때는 기다린다.
        while (turn != 1) {
            pthread_cond_wait(&cond, &mutex);
        }

        // 여기 도달했으면 지금은 자식 차례
        printf("hello child\n");
        // 다음은 부모 차례로 넘겨준다.
        turn = 0;
        // 부모에게 신호 보내기
        pthread_cond_signal(&cond);

        pthread_mutex_unlock(&mutex);
    }

    pthread_exit(NULL);
}

/* 메인(부모) 쓰레드 */
int main(void)
{
    pthread_t child;
    int status;

    // 자식 쓰레드 생성
    status = pthread_create(&child, NULL, child_thread, NULL);
    if (status != 0) {
        perror("pthread_create");
        exit(1);
    }

    // 부모도 NUM_ITER번 출력
    for (int i = 0; i < NUM_ITER; i++) {
        pthread_mutex_lock(&mutex);

        // 내 차례( turn == 0 ) 가 아닐 때는 기다린다.
        while (turn != 0) {
            pthread_cond_wait(&cond, &mutex);
        }

        // 여기 도달했으면 지금은 부모 차례
        printf("hello parent\n");
        // 다음은 자식 차례로 넘겨준다.
        turn = 1;
        // 자식에게 신호 보내기
        pthread_cond_signal(&cond);

        pthread_mutex_unlock(&mutex);
    }

    // 자식 쓰레드가 끝날 때까지 대기
    pthread_join(child, NULL);

    // (선택) 자원 정리
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);

    return 0;
}
