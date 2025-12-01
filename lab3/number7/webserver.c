/* webserver.c : TCP 소켓을 이용한 간단한 웹 서버 예제
 *  - HTTP GET /         : 간단한 HTML 페이지 전송
 *  - HTTP GET /cgi      : CGI 실행 (./cgi-bin/test.cgi)
 *  - HTTP POST /cgi     : CGI 실행 (stdin으로 body 전달)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/select.h>
#include <sys/wait.h>

#define PORT        3490      /* 사용할 포트 번호 */
#define QLEN        10        /* pending connection queue size */
#define BUF_SIZE    8192      /* 버퍼 크기 */

void handle_clnt(int client_sock);
void send_msg(int client_sock);
void send_err(int client_sock);
void handle_cgi(int client_sock,
                const char *method,
                char *path,
                char *body,
                int content_length);

int main(int argc, char *argv[])
{
    int sockfd, new_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t alen;

    fd_set readfds, activefds;
    int i, maxfd;

    /* 서버 소켓 생성 */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket() failed");
        exit(1);
    }

    /* 주소 재사용 옵션 (재실행 편하게) */
    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    /* 서버 주소 구조체 설정 */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port        = htons(PORT);

    if (bind(sockfd,
             (struct sockaddr *)&server_addr,
             sizeof(server_addr)) < 0) {
        perror("bind() failed");
        exit(1);
    }

    if (listen(sockfd, QLEN) < 0) {
        perror("listen() failed");
        exit(1);
    }

    printf("Server up and running on port %d\n", PORT);

    /* select용 fd set 초기화 */
    FD_ZERO(&activefds);
    FD_SET(sockfd, &activefds);
    maxfd = sockfd;

    /* 메인 서버 루프 */
    while (1) {
        readfds = activefds;

        if (select(maxfd + 1, &readfds, NULL, NULL, NULL) < 0) {
            perror("select() failed");
            exit(1);
        }

        /* 준비된 소켓들 처리 */
        for (i = 0; i <= maxfd; i++) {
            if (FD_ISSET(i, &readfds)) {
                if (i == sockfd) {
                    /* 새 클라이언트 접속 */
                    alen = sizeof(client_addr);
                    new_fd = accept(sockfd,
                                    (struct sockaddr *)&client_addr,
                                    &alen);
                    if (new_fd < 0) {
                        perror("accept() failed");
                        continue;
                    }
                    FD_SET(new_fd, &activefds);
                    if (new_fd > maxfd) maxfd = new_fd;

                    printf("New client: fd=%d\n", new_fd);
                } else {
                    /* 기존 클라이언트 처리 */
                    handle_clnt(i);
                    close(i);
                    FD_CLR(i, &activefds);
                }
            }
        }
    }

    close(sockfd);
    return 0;
}

/* 클라이언트 하나 처리 */
void handle_clnt(int client_sock)
{
    int recv_cnt;
    char buf[BUF_SIZE];
    char method[8], path[256];
    char *header_end, *body;
    int content_length = 0;

    /* HTTP 요청 한 번 읽기 (간단하게 1번 read한다고 가정) */
    recv_cnt = read(client_sock, buf, BUF_SIZE - 1);
    if (recv_cnt <= 0) return;
    buf[recv_cnt] = '\0';

    /* 첫 줄: METHOD PATH HTTP/... */
    if (sscanf(buf, "%7s %255s", method, path) != 2) {
        send_err(client_sock);
        return;
    }

    /* 헤더 끝 위치 (\r\n\r\n) 찾기 */
    header_end = strstr(buf, "\r\n\r\n");
    body = header_end ? header_end + 4 : NULL;

    /* POST일 때 Content-Length 읽기 (대소문자 구분없이 쓰지 않고, 정확히 이 문자열 온다고 가정) */
    if (header_end) {
        char *cl = strstr(buf, "Content-Length:");
        if (cl != NULL) {
            sscanf(cl, "Content-Length: %d", &content_length);
        }
    }

    /* /cgi 로 시작하면 CGI 로 처리 */
    if (strncmp(path, "/cgi", 4) == 0) {
        handle_cgi(client_sock, method, path, body, content_length);
        return;
    }

    /* 그 외 GET 요청은 간단한 페이지 전송 */
    if (strcmp(method, "GET") == 0) {
        send_msg(client_sock);
    }
    /* /cgi 아닌 POST는 여기서는 에러 처리 */
    else if (strcmp(method, "POST") == 0) {
        send_err(client_sock);
    }
    /* 나머지 메소드도 에러 */
    else {
        send_err(client_sock);
    }
}

/* 간단한 200 OK 응답 (정적 HTML 한 페이지) */
void send_msg(int client_sock)
{
    char protocol[]  = "HTTP/1.1 200 OK\r\n";
    char server[]    = "Server: Simple-C-Server\r\n";
    char contenttype[] = "Content-Type: text/html\r\n";
    char end[]       = "\r\n";
    char html[] =
        "<html><head><title>Hello</title></head>"
        "<body><h1>Hello World</h1>"
        "<p>Use <b>/cgi</b> for CGI test.</p>"
        "</body></html>";

    char contentlength[64];
    sprintf(contentlength, "Content-Length: %zu\r\n", strlen(html));

    write(client_sock, protocol, strlen(protocol));
    write(client_sock, server, strlen(server));
    write(client_sock, contentlength, strlen(contentlength));
    write(client_sock, contenttype, strlen(contenttype));
    write(client_sock, end, strlen(end));
    write(client_sock, html, strlen(html));
}

/* 400 Bad Request 응답 */
void send_err(int client_sock)
{
    char protocol[]  = "HTTP/1.1 400 Bad Request\r\n";
    char server[]    = "Server: Simple-C-Server\r\n";
    char contenttype[] = "Content-Type: text/html\r\n";
    char end[]       = "\r\n";
    char html[] =
        "<html><head><title>Bad Request</title></head>"
        "<body><h1>400 Bad Request</h1></body></html>";

    char contentlength[64];
    sprintf(contentlength, "Content-Length: %zu\r\n", strlen(html));

    write(client_sock, protocol, strlen(protocol));
    write(client_sock, server, strlen(server));
    write(client_sock, contentlength, strlen(contentlength));
    write(client_sock, contenttype, strlen(contenttype));
    write(client_sock, end, strlen(end));
    write(client_sock, html, strlen(html));
}

/* /cgi 요청 처리 (GET/POST 공통) */
void handle_cgi(int client_sock,
                const char *method,
                char *path,
                char *body,
                int content_length)
{
    int in_pipe[2], out_pipe[2];
    pid_t pid;
    int n;
    char buf[BUF_SIZE];

    /* 파이프 2개 생성: 부모→자식, 자식→부모 */
    if (pipe(in_pipe) < 0 || pipe(out_pipe) < 0) {
        send_err(client_sock);
        return;
    }

    pid = fork();
    if (pid < 0) {
        send_err(client_sock);
        return;
    }

    if (pid == 0) {
        /* ----- child: CGI 실행 ----- */
        dup2(in_pipe[0], STDIN_FILENO);
        dup2(out_pipe[1], STDOUT_FILENO);
        close(in_pipe[1]);
        close(out_pipe[0]);

        /* 환경변수 설정 (간단 버전) */
        setenv("REQUEST_METHOD", method, 1);

        if (strcmp(method, "POST") == 0 && content_length > 0) {
            char clbuf[32];
            sprintf(clbuf, "%d", content_length);
            setenv("CONTENT_LENGTH", clbuf, 1);
        }

        /* 여기서는 무조건 ./cgi-bin/test.cgi 하나만 실행 */
        execl("./cgi-bin/test.cgi", "./cgi-bin/test.cgi", (char *)NULL);

        /* execl 실패시 그냥 종료 */
        exit(1);
    } else {
        /* ----- parent: 클라이언트/CGI 중계 ----- */
        close(in_pipe[0]);
        close(out_pipe[1]);

        /* POST일 경우 body를 CGI stdin으로 전달
           (주의: body 전체가 한 번에 들어왔다고 가정) */
        if (strcmp(method, "POST") == 0 &&
            body != NULL && content_length > 0) {
            write(in_pipe[1], body, content_length);
        }
        close(in_pipe[1]);

        /* HTTP 응답 헤더 (CGI 출력은 body로 취급) */
        {
            const char *hdr =
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/html\r\n\r\n";
            write(client_sock, hdr, strlen(hdr));
        }

        /* CGI가 stdout으로 쓰는 내용 -> 클라이언트로 그대로 전송 */
        while ((n = read(out_pipe[0], buf, BUF_SIZE)) > 0) {
            write(client_sock, buf, n);
        }
        close(out_pipe[0]);

        wait(NULL);
    }
}
