#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>

#define PORT 3490
#define MAXBUF 512
#define MAXMSG 1024

int main(int argc, char *argv[])
{
    int sock;
    struct sockaddr_in serv_addr;
    fd_set readfds;
    char sendbuf[MAXBUF];
    char recvbuf[MAXBUF];

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <SERVER IP> <NICKNAME>\n", argv[0]);
        exit(1);
    }

    /* 1. 소켓 생성 */
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(1);
    }

    /* 2. 서버 주소 설정 */
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port   = htons(PORT);
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]); // 예: 127.0.0.1

    /* 3. 서버 접속 */
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect");
        exit(1);
    }

    printf("Connected to %s:%d as %s\n", argv[1], PORT, argv[2]);
    printf("Type messages and press Enter. '/quit' to exit.\n");

    /* 4. 메인 루프: stdin + 소켓을 동시에 select로 감시 */
    while (1) {
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds); // 키보드 입력
        FD_SET(sock, &readfds);         // 서버 소켓

        int maxfd = (sock > STDIN_FILENO ? sock : STDIN_FILENO);

        if (select(maxfd + 1, &readfds, NULL, NULL, NULL) < 0) {
            perror("select");
            exit(1);
        }

        /* 4-1. 키보드 입력이 있으면 서버로 전송 */
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            if (fgets(sendbuf, sizeof(sendbuf), stdin) == NULL) {
                printf("Input EOF, exiting.\n");
                break;
            }

            if (strncmp(sendbuf, "/quit", 5) == 0)
                break;

            char msg[MAXMSG];
            snprintf(msg, sizeof(msg), "[%s] %s", argv[2], sendbuf);

            if (send(sock, msg, strlen(msg), 0) == -1) {
                perror("send");
                break;
            }
        }

        /* 4-2. 서버로부터 메시지 수신 */
        if (FD_ISSET(sock, &readfds)) {
            int nbytes = recv(sock, recvbuf, sizeof(recvbuf) - 1, 0);
            if (nbytes <= 0) {
                if (nbytes == 0) {
                    printf("Server closed connection.\n");
                } else {
                    perror("recv");
                }
                break;
            }
            recvbuf[nbytes] = '\0';
            printf("%s", recvbuf);
            fflush(stdout);
        }
    }

    close(sock);
    return 0;
}
