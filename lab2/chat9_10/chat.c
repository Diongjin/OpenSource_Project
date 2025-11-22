// chat_pipe.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#define BUF_SIZE 1024

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "사용법: %s A 또는 %s B\n", argv[0], argv[0]);
        fprintf(stderr, "예:\n");
        fprintf(stderr, "  터미널1: %s A\n", argv[0]);
        fprintf(stderr, "  터미널2: %s B\n", argv[0]);
        return 1;
    }

    const char *role = argv[1];       // "A" 또는 "B"
    const char *fifo_a = "fifo_AtoB"; // A -> B
    const char *fifo_b = "fifo_BtoA"; // B -> A

    const char *send_fifo;
    const char *recv_fifo;

    int send_fd, recv_fd;
    char buf[BUF_SIZE];

    // 역할에 따라 보낼 FIFO / 받을 FIFO 설정
    if (strcmp(role, "A") == 0) {
        send_fifo = fifo_a; // 내가 쓰는 쪽
        recv_fifo = fifo_b; // 내가 읽는 쪽
    } else if (strcmp(role, "B") == 0) {
        send_fifo = fifo_b;
        recv_fifo = fifo_a;
    } else {
        fprintf(stderr, "역할은 A 또는 B만 가능합니다.\n");
        return 1;
    }

    // FIFO 두 개 생성 (이미 있으면 EEXIST 무시)
    if (mkfifo(fifo_a, 0666) == -1 && errno != EEXIST) {
        perror("mkfifo fifo_AtoB");
        return 1;
    }
    if (mkfifo(fifo_b, 0666) == -1 && errno != EEXIST) {
        perror("mkfifo fifo_BtoA");
        return 1;
    }

    printf("[%s] 파이프 준비 완료. 상대방 프로그램을 실행하세요.\n", role);

    // ★ 데드락 안 걸리게 "열기 순서"를 다르게 함 ★
    if (strcmp(role, "A") == 0) {
        // A: 먼저 내가 쓸 파이프를 쓰기 전용으로 열고, 그다음 읽기용 열기
        send_fd = open(send_fifo, O_WRONLY);
        if (send_fd == -1) {
            perror("open send_fifo");
            return 1;
        }
        recv_fd = open(recv_fifo, O_RDONLY);
        if (recv_fd == -1) {
            perror("open recv_fifo");
            close(send_fd);
            return 1;
        }
    } else {
        // B: 먼저 읽을 파이프를 읽기 전용으로 열고, 그다음 쓸 파이프 열기
        recv_fd = open(recv_fifo, O_RDONLY);
        if (recv_fd == -1) {
            perror("open recv_fifo");
            return 1;
        }
        send_fd = open(send_fifo, O_WRONLY);
        if (send_fd == -1) {
            perror("open send_fifo");
            close(recv_fd);
            return 1;
        }
    }

    printf("[%s] 채팅 시작! 종료하려면 'exit'을 입력하세요.\n", role);

    // ----- 채팅 루프 -----
    // A가 먼저 말하고 B가 답하는 "번갈아 채팅" 버전
    while (1) {
        // 1) 내가 먼저 말하는 쪽인지, 먼저 듣는 쪽인지 분기
        if (strcmp(role, "A") == 0) {
            // A: 먼저 입력하고 보내고, 그 다음 상대 말을 받음
            printf("A> ");
            fflush(stdout);

            if (fgets(buf, sizeof(buf), stdin) == NULL) {
                printf("\n입력 종료. 채팅을 종료합니다.\n");
                break;
            }
            buf[strcspn(buf, "\n")] = '\0';

            if (strcmp(buf, "exit") == 0) {
                printf("채팅 종료.\n");
                break;
            }

            // 보내기
            if (write(send_fd, buf, strlen(buf) + 1) == -1) { // +1: '\0' 포함
                perror("write");
                break;
            }

            // 상대방 메시지 받기
            ssize_t n = read(recv_fd, buf, sizeof(buf));
            if (n == -1) {
                perror("read");
                break;
            } else if (n == 0) {
                printf("상대방이 채팅을 종료했습니다.\n");
                break;
            }
            printf("B> %s\n", buf);
        } else {
            // B: 먼저 상대(A)의 말을 받고, 그 다음 내가 답장
            ssize_t n = read(recv_fd, buf, sizeof(buf));
            if (n == -1) {
                perror("read");
                break;
            } else if (n == 0) {
                printf("상대방이 채팅을 종료했습니다.\n");
                break;
            }
            printf("A> %s\n", buf);

            printf("B> ");
            fflush(stdout);

            if (fgets(buf, sizeof(buf), stdin) == NULL) {
                printf("\n입력 종료. 채팅을 종료합니다.\n");
                break;
            }
            buf[strcspn(buf, "\n")] = '\0';

            if (strcmp(buf, "exit") == 0) {
                printf("채팅 종료.\n");
                break;
            }

            if (write(send_fd, buf, strlen(buf) + 1) == -1) {
                perror("write");
                break;
            }
        }
    }

    close(send_fd);
    close(recv_fd);

    // FIFO 파일은 과제 채점 편하게 남겨둬도 되고,
    // 지우고 싶으면 아래 두 줄 주석 해제
    // unlink(fifo_a);
    // unlink(fifo_b);

    return 0;
}
