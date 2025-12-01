#define _GNU_SOURCE     // ★ SIGIO, FASYNC 사용에 필요

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/file.h>
#include <signal.h>
#include <errno.h>
#define MAXBUFSIZE 255
#define SERV_PORT 5000
#define SLEEP_TIME 5

void UseIdleTime();
void SocketHandler(int signalType);
int sock;
int main(int argc, char *argv[])
{
    struct sockaddr_in serv_addr;
    unsigned short serv_port = SERV_PORT;
    struct sigaction handler;
    if ((sock = socket(AF_INET, SOCK_DGRAM,
                       IPPROTO_UDP)) < 0)
    {
        perror("socket() failed");
        exit(1);
    }
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(serv_port);
    if (bind(sock, (struct sockaddr *)&serv_addr,
             sizeof(serv_addr)) < 0)
    {
        perror("bind() failed");
        exit(1);
    }
    handler.sa_handler = SocketHandler;
    if (sigfillset(&handler.sa_mask) < 0)
    {
        perror("sigfillset() failed");
        exit(1);
    }
    handler.sa_flags = 0;
    /* SIGIO 핸들러 연결 */
    if (sigaction(SIGIO, &handler, 0) < 0)
    {
        perror("sigaction() failed for SIGIO");
        exit(1);
    }
    /* 서버 소켓에 대한 이벤트를 받을 프로세스 연결 */
    if (fcntl(sock, F_SETOWN, getpid()) < 0)
    {
        perror("Unable to set process owner to us");
        exit(1);
    }
    /* 서버 소켓을 넌블로킹&비동기 모드로 설정 */
    if (fcntl(sock, F_SETFL, O_NONBLOCK | FASYNC) < 0)
    {
        perror("Unable to put client sock into nonblocking/async mode");
        exit(1);
    }
    for (;;)
        UseIdleTime();
}
void UseIdleTime()
{
    printf("SIGIO 관련 이벤트가 없으면 이 함수가 실행됨.\n");
    printf("소켓 관련되지 않은 일 처리 가능함.\n\n");
    sleep(SLEEP_TIME);
}
void SocketHandler(int signalType)
{
    struct sockaddr_in clnt_addr;
    unsigned int clntLen;
    int recvMsgSize;
    char buf[MAXBUFSIZE];
    do
    {
        clntLen = sizeof(clnt_addr);
        if ((recvMsgSize = recvfrom(sock, buf,
                                    MAXBUFSIZE, 0, (struct sockaddr *)&clnt_addr,
                                    &clntLen)) < 0)
        {
            if (errno != EWOULDBLOCK)
            {
                perror("recvfrom() failed");
                exit(1);
            }
        }
        else
        {
            printf("Handling client %s\n",
                   inet_ntoa(clnt_addr.sin_addr));
            if (sendto(sock, buf, recvMsgSize, 0,
                       (struct sockaddr *)&clnt_addr,
                       sizeof(clnt_addr)) != recvMsgSize)
            {
                perror("sendto() failed");
                exit(1);
            }
        }
    } while (recvMsgSize >= 0);
}