/* boundedbuffer.c */
/* 제한 버퍼(BOUND BUFFER) 생산자-소비자 문제 - pthread 조건변수 버전 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

#define BUFFER_SIZE      5      // 버퍼 크기
#define NUM_PRODUCERS    2      // 생산자 쓰레드 수 (2 이상)
#define NUM_CONSUMERS    2      // 소비자 쓰레드 수 (2 이상)
#define PRODUCE_COUNT    10     // 각 생산자가 생산할 아이템 개수

/* 공유 버퍼 구조체 */
typedef struct {
    int buffer[BUFFER_SIZE];
    int in;              // 다음에 쓸 위치
    int out;             // 다음에 읽을 위치
    int count;           // 현재 버퍼 안의 아이템 개수
    pthread_mutex_t mutex;
    pthread_cond_t  not_full;
    pthread_cond_t  not_empty;
} bounded_buffer_t;

bounded_buffer_t bb = {
    .buffer = {0},
    .in = 0,
    .out = 0,
    .count = 0,
    .mutex = PTHREAD_MUTEX_INITIALIZER,
    .not_full = PTHREAD_COND_INITIALIZER,
    .not_empty = PTHREAD_COND_INITIALIZER
};

/* 생산자가 아이템 하나 만드는 함수 */
int produce_item(int id, int seq)
{
    int item = (int)(100.0 * rand() / (RAND_MAX + 1.0));
    usleep(100 * 1000); // 0.1초 정도 쉼 (생산 시간 흉내)
    printf("[Producer %d] produce #%d: %d\n", id, seq, item);
    return item;
}

/* 버퍼에 아이템 삽입 */
void insert_item(int item)
{
    pthread_mutex_lock(&bb.mutex);

    // 버퍼가 꽉 찼으면 not_full 신호를 기다린다.
    while (bb.count == BUFFER_SIZE) {
        pthread_cond_wait(&bb.not_full, &bb.mutex);
    }

    bb.buffer[bb.in] = item;
    bb.in = (bb.in + 1) % BUFFER_SIZE;
    bb.count++;

    // 소비자에게 데이터가 생겼다고 알려준다.
    pthread_cond_signal(&bb.not_empty);
    pthread_mutex_unlock(&bb.mutex);
}

/* 소비자가 아이템 하나 소비하는 함수 */
void consume_item(int id, int seq, int item)
{
    usleep(150 * 1000); // 0.15초 정도 쉼 (소비 시간 흉내)
    printf("\t[Consumer %d] consume #%d: %d\n", id, seq, item);
}

/* 버퍼에서 아이템 제거 */
int remove_item(void)
{
    int item;

    pthread_mutex_lock(&bb.mutex);

    // 버퍼가 비어 있으면 not_empty 신호를 기다린다.
    while (bb.count == 0) {
        pthread_cond_wait(&bb.not_empty, &bb.mutex);
    }

    item = bb.buffer[bb.out];
    bb.out = (bb.out + 1) % BUFFER_SIZE;
    bb.count--;

    // 생산자에게 공간이 생겼다고 알려준다.
    pthread_cond_signal(&bb.not_full);
    pthread_mutex_unlock(&bb.mutex);

    return item;
}

/* 생산자 쓰레드 함수 */
void *producer(void *arg)
{
    int id = *(int *)arg;

    for (int i = 0; i < PRODUCE_COUNT; i++) {
        int item = produce_item(id, i);
        insert_item(item);
    }

    printf("[Producer %d] finished.\n", id);
    pthread_exit(NULL);
}

/* 소비자 쓰레드 함수 */
void *consumer(void *arg)
{
    int id = *(int *)arg;

    // 전체 생산량을 소비자 수로 나눈 만큼 소비
    int my_count = (NUM_PRODUCERS * PRODUCE_COUNT) / NUM_CONSUMERS;

    for (int i = 0; i < my_count; i++) {
        int item = remove_item();
        consume_item(id, i, item);
    }

    printf("[Consumer %d] finished.\n", id);
    pthread_exit(NULL);
}

int main(void)
{
    pthread_t prod_tid[NUM_PRODUCERS];
    pthread_t cons_tid[NUM_CONSUMERS];
    int prod_id[NUM_PRODUCERS];
    int cons_id[NUM_CONSUMERS];

    srand((unsigned int)time(NULL));

    // 생산자 쓰레드 생성
    for (int i = 0; i < NUM_PRODUCERS; i++) {
        prod_id[i] = i;
        if (pthread_create(&prod_tid[i], NULL, producer, &prod_id[i]) != 0) {
            perror("생산자 쓰레드 생성");
            exit(1);
        }
    }

    // 소비자 쓰레드 생성
    for (int i = 0; i < NUM_CONSUMERS; i++) {
        cons_id[i] = i;
        if (pthread_create(&cons_tid[i], NULL, consumer, &cons_id[i]) != 0) {
            perror("소비자 쓰레드 생성");
            exit(1);
        }
    }

    // 생산자 쓰레드 종료 대기
    for (int i = 0; i < NUM_PRODUCERS; i++) {
        pthread_join(prod_tid[i], NULL);
    }

    // 소비자 쓰레드 종료 대기
    for (int i = 0; i < NUM_CONSUMERS; i++) {
        pthread_join(cons_tid[i], NULL);
    }

    printf("All producers and consumers finished.\n");
    return 0;
}
