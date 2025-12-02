#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUF_SIZE 1024
#define DEFAULT_PORT 5555

typedef struct {
    int sock;
    int registered;          // 0: 아직 /join 안 함, 1: 등록됨
    char name[32];           // 닉네임
    char room[32];           // 방 이름
} Client;

static Client clients[FD_SETSIZE];

void init_clients() {
    for (int i = 0; i < FD_SETSIZE; i++) {
        clients[i].sock = -1;
        clients[i].registered = 0;
        clients[i].name[0] = '\0';
        clients[i].room[0] = '\0';
    }
}

int add_client(int sock) {
    for (int i = 0; i < FD_SETSIZE; i++) {
        if (clients[i].sock < 0) {
            clients[i].sock = sock;
            clients[i].registered = 0;
            clients[i].name[0] = '\0';
            clients[i].room[0] = '\0';
            return i;
        }
    }
    return -1; // 꽉 참
}

void remove_client(int idx, fd_set *allset) {
    if (clients[idx].sock >= 0) {
        printf("client disconnected: fd=%d, name=%s, room=%s\n",
               clients[idx].sock,
               clients[idx].name,
               clients[idx].room);
        close(clients[idx].sock);
        FD_CLR(clients[idx].sock, allset);
        clients[idx].sock = -1;
        clients[idx].registered = 0;
        clients[idx].name[0] = '\0';
        clients[idx].room[0] = '\0';
    }
}

/* 같은 방에 있는 클라이언트에게만 메시지 브로드캐스트 */
void broadcast_message(const char *room,
                       int from_idx,
                       const char *msg)
{
    char buf[BUF_SIZE];

    snprintf(buf, sizeof(buf),
             "[from %s %s] %s\n",
             clients[from_idx].room,
             clients[from_idx].name,
             msg);

    for (int i = 0; i < FD_SETSIZE; i++) {
        if (i == from_idx) continue;
        if (clients[i].sock < 0) continue;
        if (!clients[i].registered) continue;
        if (strcmp(clients[i].room, room) != 0) continue;

        write(clients[i].sock, buf, strlen(buf));
    }

    // 보낸 사람에게도 echo 하고 싶으면 아래 활성화
    // write(clients[from_idx].sock, buf, strlen(buf));
}

/* 한 클라이언트에서 온 한 줄을 처리 */
void handle_line(int idx, char *line)
{
    int sock = clients[idx].sock;

    // 줄 끝 \r\n / \n 제거
    size_t len = strlen(line);
    while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) {
        line[--len] = '\0';
    }

    if (len == 0) return;

    if (strncmp(line, "/join", 5) == 0) {
        char name[32], room[32];
        if (sscanf(line, "/join %31s %31s", name, room) != 2) {
            const char *err = "ERR Usage: /join <name> <room>\n";
            write(sock, err, strlen(err));
            return;
        }

        // 이미 등록되어 있어도 그냥 덮어쓰기 (간단 버전)
        strncpy(clients[idx].name, name, sizeof(clients[idx].name)-1);
        clients[idx].name[sizeof(clients[idx].name)-1] = '\0';

        strncpy(clients[idx].room, room, sizeof(clients[idx].room)-1);
        clients[idx].room[sizeof(clients[idx].room)-1] = '\0';

        clients[idx].registered = 1;

        char okmsg[BUF_SIZE];
        snprintf(okmsg, sizeof(okmsg),
                 "OK Joined as %s in room %s\n",
                 clients[idx].name,
                 clients[idx].room);
        write(sock, okmsg, strlen(okmsg));

        printf("client fd=%d joined: name=%s, room=%s\n",
               sock, clients[idx].name, clients[idx].room);
    }
    else if (strncmp(line, "/msg", 4) == 0) {
        if (!clients[idx].registered) {
            const char *err = "ERR Please /join first\n";
            write(sock, err, strlen(err));
            return;
        }

        char *msg = line + 4;
        while (*msg == ' ') msg++;

        if (*msg == '\0') {
            const char *err = "ERR Usage: /msg <message>\n";
            write(sock, err, strlen(err));
            return;
        }

        broadcast_message(clients[idx].room, idx, msg);
    }
    else {
        // 알 수 없는 명령어 → 그냥 안내
        const char *err = "ERR Unknown command. Use /join or /msg\n";
        write(sock, err, strlen(err));
    }
}

int main(int argc, char *argv[])
{
    int listenfd, connfd;
    int port = (argc >= 2) ? atoi(argv[1]) : DEFAULT_PORT;

    struct sockaddr_in servaddr, cliaddr;
    socklen_t clilen;

    fd_set allset, rset;
    int maxfd, nready;

    init_clients();

    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(1);
    }

    int opt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family      = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port        = htons(port);

    if (bind(listenfd, (struct sockaddr*)&servaddr,
             sizeof(servaddr)) < 0) {
        perror("bind");
        exit(1);
    }

    if (listen(listenfd, 10) < 0) {
        perror("listen");
        exit(1);
    }

    printf("Chat server listening on port %d\n", port);

    FD_ZERO(&allset);
    FD_SET(listenfd, &allset);
    maxfd = listenfd;

    while (1) {
        rset = allset;

        nready = select(maxfd + 1, &rset, NULL, NULL, NULL);
        if (nready < 0) {
            if (errno == EINTR) continue;
            perror("select");
            exit(1);
        }

        // 새 연결 처리
        if (FD_ISSET(listenfd, &rset)) {
            clilen = sizeof(cliaddr);
            connfd = accept(listenfd,
                            (struct sockaddr*)&cliaddr,
                            &clilen);
            if (connfd < 0) {
                perror("accept");
                continue;
            }

            int idx = add_client(connfd);
            if (idx < 0) {
                const char *msg = "Server full\n";
                write(connfd, msg, strlen(msg));
                close(connfd);
            } else {
                FD_SET(connfd, &allset);
                if (connfd > maxfd) maxfd = connfd;

                char greet[] =
                    "Welcome to Chat Server!\n"
                    "Use: /join <name> <room>\n";
                write(connfd, greet, strlen(greet));

                printf("new client connected: fd=%d (index=%d)\n",
                       connfd, idx);
            }

            if (--nready <= 0) continue;
        }

        // 기존 클라이언트 처리
        for (int i = 0; i < FD_SETSIZE; i++) {
            if (clients[i].sock < 0) continue;
            int sock = clients[i].sock;

            if (FD_ISSET(sock, &rset)) {
                char buf[BUF_SIZE];
                int n = read(sock, buf, sizeof(buf) - 1);

                if (n <= 0) {
                    // 연결 종료
                    remove_client(i, &allset);
                } else {
                    buf[n] = '\0';
                    // 이 예제는 한 번에 한 줄만 온다고 가정 (단순화)
                    handle_line(i, buf);
                }

                if (--nready <= 0) break;
            }
        }
    }

    close(listenfd);
    return 0;
}