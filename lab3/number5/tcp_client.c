#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>  

#define PORT 3490
#define MAXDATASIZE 100

int main(int argc, char *argv[])
{
	int csock, numbytes;
	char buf[MAXDATASIZE];
	struct sockaddr_in serv_addr;
	int len;
	if (argc != 3)
	{
		fprintf(stderr, "usage: tcp_client <SERVER IP> <ECHO STRING>\n");
		exit(1);
	}

	if ((csock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("socket");
		exit(1);
	}

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
	serv_addr.sin_port = htons(PORT);

	/*서버 연결*/
	if (connect(csock, (struct sockaddr *)&serv_addr,
				sizeof(struct sockaddr)) == -1)
	{
		perror("connect");
		exit(1);
	}
	/* 메시지 송수신 */
	memset(buf, 0, MAXDATASIZE);
	strcpy(buf, argv[2]);
	len = strlen(buf);

	if(send(csock, buf, len, 0) != len){
		fprintf(stderr, "send error...\n");
		exit(1);
	}
	memset(buf, 0, MAXDATASIZE);
	if((numbytes = recv(csock, buf, MAXDATASIZE,0)) == -1){
		perror("recv");
		exit(1);
	}

	printf("Received: %s\n", buf);

	close(csock);
}