// chat_mq.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MSG_LEN 1024
#define KEY_FILE "chat_key"

// 메시지 큐에 넣을 구조체
struct chat_msg {
    long mtype;            // 누가 받을지 구분하는 타입 (1 또는 2)
    char text[MSG_LEN];    // 실제 메시지
};

// mtype을 뺀 나머지 크기만 전송/수신에 사용
#define MSG_DATA_SIZE (sizeof(struct chat_msg) - sizeof(long))

// ftok에 사용할 key 파일이 없으면 생성
static void ensure_key_file(void) {
    if (access(KEY_FILE, F_OK) == -1) {
        int fd = open(KEY_FILE, O_CREAT | O_RDWR, 0666);
        if (fd == -1) {
            perror("open(KEY_FILE)");
            exit(1);
        }
        close(fd);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "사용법: %s [참가자번호]\n", argv[0]);
        fprintf(stderr, "예시:\n");
        fprintf(stderr, "  터미널1: %s 1\n", argv[0]);
        fprintf(stderr, "  터미널2: %s 2\n", argv[0]);
        return 1;
    }

    int me = atoi(argv[1]);   // 내 번호 (1 또는 2)
    if (me != 1 && me != 2) {
        fprintf(stderr, "참가자 번호는 1 또는 2만 가능합니다.\n");
        return 1;
    }

    long my_type   = me;             // 내가 받을 메시지 타입
    long peer_type = (me == 1) ? 2 : 1;  // 상대가 받을 메시지 타입

    // 1. key 파일 준비 + ftok로 key 생성
    ensure_key_file();

    key_t key = ftok(KEY_FILE, 65);
    if (key == -1) {
        perror("ftok");
        return 1;
    }

    // 2. 메시지 큐 접근/생성
    int qid = msgget(key, 0666 | IPC_CREAT);
    if (qid == -1) {
        perror("msgget");
        return 1;
    }

    printf("=== 메시지 큐 채팅 시작 (나: %d, 상대: %ld) ===\n", me, peer_type);
    printf("종료하려면 'exit'을 입력하세요.\n");

    struct chat_msg msg;
    char input[MSG_LEN];

    while (1) {
        // ----- 1) 내가 보낼 메시지 입력 -----
        printf("나(%d)> ", me);
        fflush(stdout);

        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("\n입력 종료. 채팅을 종료합니다.\n");
            break;
        }

        // 개행 문자 제거
        input[strcspn(input, "\n")] = '\0';

        // exit 입력 시 종료
        if (strcmp(input, "exit") == 0) {
            printf("채팅 종료.\n");
            break;
        }

        // ----- 2) 메시지 전송 -----
        msg.mtype = peer_type;               // 상대가 받을 타입
        strncpy(msg.text, input, MSG_LEN);
        msg.text[MSG_LEN - 1] = '\0';

        if (msgsnd(qid, &msg, MSG_DATA_SIZE, 0) == -1) {
            perror("msgsnd");
            break;
        }

        // ----- 3) 상대 메시지 수신 -----
        if (msgrcv(qid, &msg, MSG_DATA_SIZE, my_type, 0) == -1) {
            perror("msgrcv");
            break;
        }

        printf("상대(%ld)> %s\n", msg.mtype, msg.text);
    }

    // 참가자 1이 큐 삭제 담당
    if (me == 1) {
        if (msgctl(qid, IPC_RMID, NULL) == -1) {
            perror("msgctl(IPC_RMID)");
        } else {
            printf("메시지 큐 삭제 완료.\n");
        }
    }

    return 0;
}
