#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>

#define PORT 3490          // 서버 포트
#define QLEN 10            // listen 대기 큐 크기
#define MAXBUF 512

int main(void)
{
    int listenfd, newfd;
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t addrlen;
    fd_set activefds, readfds;
    int maxfd, i, nbytes;
    char buf[MAXBUF];

    /* 1. 리슨 소켓 생성 */
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(1);
    }

    /* SO_REUSEADDR 설정 (재실행 편하게) */
    int yes = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
        perror("setsockopt");
        exit(1);
    }

    /* 2. 주소 설정 */
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port   = htons(PORT);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("bind");
        exit(1);
    }

    /* 3. listen */
    if (listen(listenfd, QLEN) < 0) {
        perror("listen");
        exit(1);
    }

    printf("SERVER: listening on port %d ...\n", PORT);

    /* 4. fd_set 초기화 */
    FD_ZERO(&activefds);
    FD_SET(listenfd, &activefds);
    maxfd = listenfd;

    /* 5. 메인 루프: select로 다중 클라이언트 처리 */
    while (1) {
        readfds = activefds;   // select용 복사본

        if (select(maxfd + 1, &readfds, NULL, NULL, NULL) < 0) {
            perror("select");
            exit(1);
        }

        /* 어떤 fd에 이벤트가 있는지 확인 */
        for (i = 0; i <= maxfd; i++) {
            if (!FD_ISSET(i, &readfds))
                continue;

            /* 새 연결 도착 */
            if (i == listenfd) {
                addrlen = sizeof(cli_addr);
                newfd = accept(listenfd, (struct sockaddr *)&cli_addr, &addrlen);
                if (newfd < 0) {
                    perror("accept");
                    continue;
                }

                printf("SERVER: new client %s, fd=%d\n",
                       inet_ntoa(cli_addr.sin_addr), newfd);

                FD_SET(newfd, &activefds);
                if (newfd > maxfd)
                    maxfd = newfd;
            }
            /* 기존 클라이언트에서 데이터 도착 */
            else {
                memset(buf, 0, sizeof(buf));
                nbytes = recv(i, buf, sizeof(buf), 0);
                if (nbytes <= 0) {
                    if (nbytes == 0) {
                        printf("SERVER: client fd=%d disconnected\n", i);
                    } else {
                        perror("recv");
                    }
                    close(i);
                    FD_CLR(i, &activefds);
                } else {
                    /* 보기 좋게 개행 추가 */
                    if (buf[nbytes - 1] != '\n')
                        buf[nbytes++] = '\n';

                    printf("[fd %d] %s", i, buf);

                    /* 채팅: 보낸 클라이언트를 제외한 모든 클라이언트에게 브로드캐스트 */
                    for (int j = 0; j <= maxfd; j++) {
                        if (j != listenfd && j != i && FD_ISSET(j, &activefds)) {
                            if (send(j, buf, nbytes, 0) == -1) {
                                perror("send");
                            }
                        }
                    }
                }
            }
        }
    }

    close(listenfd);
    return 0;
}
