// chat_server.c (파일 전송 지원 버전)
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
#include <locale.h>

#define PORT    3490
#define QLEN    10
#define MAXBUF  4096
#define MAXNAME 32
#define MAXROOM 32

// fd 번호별 사용자/방 정보
static int  registered[FD_SETSIZE];          // 0: 미등록, 1: /join 완료
static char nicknames[FD_SETSIZE][MAXNAME];  // 닉네임
static char rooms[FD_SETSIZE][MAXROOM];      // 방 이름
// fd 번호별 파일 수신 상태 (이 fd로부터 앞으로 몇 바이트의 파일 데이터가 올 예정인가?)
static long file_remain[FD_SETSIZE];         // 0이면 평상시, >0이면 파일 데이터 모드

void init_clients() {
    for (int i = 0; i < FD_SETSIZE; i++) {
        registered[i] = 0;
        nicknames[i][0] = '\0';
        rooms[i][0] = '\0';
        file_remain[i] = 0;
    }
}

// /join, /msg, /file 헤더 처리
void handle_line(int fd, fd_set *activefds, char *line) {
    char buf[MAXBUF];

    // 줄 끝의 \r\n 제거
    size_t len = strlen(line);
    while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) {
        line[--len] = '\0';
    }
    if (len == 0) return;

    // /join <name> <room>
    if (strncmp(line, "/join", 5) == 0) {
        char name[MAXNAME], room[MAXROOM];
        if (sscanf(line, "/join %31s %31s", name, room) != 2) {
            const char *err = "ERR Usage: /join <name> <room>\n";
            send(fd, err, strlen(err), 0);
            return;
        }
        strncpy(nicknames[fd], name, MAXNAME-1);
        nicknames[fd][MAXNAME-1] = '\0';
        strncpy(rooms[fd], room, MAXROOM-1);
        rooms[fd][MAXROOM-1] = '\0';
        registered[fd] = 1;

        snprintf(buf, sizeof(buf),
                 "OK Joined as %s in room %s\n",
                 nicknames[fd], rooms[fd]);
        send(fd, buf, strlen(buf), 0);

        printf("SERVER: fd=%d joined name=%s room=%s\n",
               fd, nicknames[fd], rooms[fd]);
    }
    // /msg <message>
    else if (strncmp(line, "/msg", 4) == 0) {
        if (!registered[fd]) {
            const char *err = "ERR Please /join first.\n";
            send(fd, err, strlen(err), 0);
            return;
        }
        char *msg = line + 4;
        while (*msg == ' ') msg++;   // 앞 공백 제거

        if (*msg == '\0') {
            const char *err = "ERR Usage: /msg <message>\n";
            send(fd, err, strlen(err), 0);
            return;
        }

        // 같은 방에 있는 사용자에게 브로드캐스트 (보낸 사람 포함)
        char out[MAXBUF];
        snprintf(out, sizeof(out),
                 "[room %s][%s] %s\n",
                 rooms[fd], nicknames[fd], msg);

        for (int j = 0; j < FD_SETSIZE; j++) {
            if (!FD_ISSET(j, activefds)) continue;
            if (!registered[j]) continue;
            if (strcmp(rooms[j], rooms[fd]) != 0) continue;

            if (send(j, out, strlen(out), 0) == -1) {
                perror("send");
            }
        }
    }
    // /file <filename> <size>
    else if (strncmp(line, "/file", 5) == 0) {
        if (!registered[fd]) {
            const char *err = "ERR Please /join first.\n";
            send(fd, err, strlen(err), 0);
            return;
        }

        char filename[256];
        long size = 0;
        if (sscanf(line, "/file %255s %ld", filename, &size) != 2 || size <= 0) {
            const char *err = "ERR Usage: /file <filename> <size>\n";
            send(fd, err, strlen(err), 0);
            return;
        }

        // 앞으로 이 fd에서 size 바이트만큼 파일 데이터가 올 것이다
        file_remain[fd] = size;

        // 같은 방의 모든 클라이언트에게 파일 수신 헤더 전송
        // 형식: FILE <보낸닉> <파일이름> <크기>
        char header[MAXBUF];
        snprintf(header, sizeof(header),
                 "FILE %s %s %ld\n",
                 nicknames[fd], filename, size);

        for (int j = 0; j < FD_SETSIZE; j++) {
            if (!FD_ISSET(j, activefds)) continue;
            if (!registered[j]) continue;
            if (strcmp(rooms[j], rooms[fd]) != 0) continue;

            if (send(j, header, strlen(header), 0) == -1) {
                perror("send header");
            }
        }

        printf("SERVER: fd=%d sending file '%s' (%ld bytes) to room %s\n",
               fd, filename, size, rooms[fd]);
        // 실제 파일 데이터는 다음 recv()에서 처리
    }
    else {
        const char *err = "ERR Unknown command. Use /join, /msg, /file.\n";
        send(fd, err, strlen(err), 0);
    }
}

int main(void)
{
    setlocale(LC_ALL, "");

    int listenfd, newfd;
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t addrlen;
    fd_set activefds, readfds;
    int maxfd, i, nbytes;
    char buf[MAXBUF];

    init_clients();

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

    /* 5. 메인 루프 */
    while (1) {
        readfds = activefds;

        if (select(maxfd + 1, &readfds, NULL, NULL, NULL) < 0) {
            perror("select");
            exit(1);
        }

        for (i = 0; i <= maxfd; i++) {
            if (!FD_ISSET(i, &readfds))
                continue;

            /* 새 연결 */
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

                registered[newfd] = 0;
                nicknames[newfd][0] = '\0';
                rooms[newfd][0] = '\0';
                file_remain[newfd] = 0;
            }
            /* 기존 클라이언트 */
            else {
                nbytes = recv(i, buf, sizeof(buf), 0);
                if (nbytes <= 0) {
                    if (nbytes == 0)
                        printf("SERVER: client fd=%d disconnected\n", i);
                    else
                        perror("recv");

                    close(i);
                    FD_CLR(i, &activefds);
                    registered[i] = 0;
                    nicknames[i][0] = '\0';
                    rooms[i][0] = '\0';
                    file_remain[i] = 0;
                } else {
                    // 파일 데이터 모드인지 체크
                    if (file_remain[i] > 0) {
                        long to_forward = nbytes;
                        if (to_forward > file_remain[i])
                            to_forward = file_remain[i];

                        // 같은 방의 사용자에게 그대로 전달
                        for (int j = 0; j < FD_SETSIZE; j++) {
                            if (!FD_ISSET(j, &activefds)) continue;
                            if (!registered[j]) continue;
                            if (strcmp(rooms[j], rooms[i]) != 0) continue;

                            if (send(j, buf, to_forward, 0) == -1) {
                                perror("send file data");
                            }
                        }

                        file_remain[i] -= to_forward;
                        if (file_remain[i] <= 0) {
                            printf("SERVER: file transfer from fd=%d done\n", i);
                        }
                    } else {
                        // 평상시 텍스트 명령 처리
                        buf[nbytes] = '\0';
                        handle_line(i, &activefds, buf);
                    }
                }
            }
        }
    }

    close(listenfd);
    return 0;
}