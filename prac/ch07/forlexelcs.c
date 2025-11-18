#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
int main()
{

	pid_t pid;
	pid = fork();

	if (pid == 0) {
		/* 자식 프로세스가 execl 호출 */
		execl("/bin/ls", "ls", "-l", (char *) 0);
		perror("execl failed");

	}	
       else if (pid > 0) {
		/* 자식이 끝날 때까지 수행을 일시 중단하기 위해 wait 호출 */
		wait((int *) 0);
		printf("ls completed\n");
		exit(0);

       } else
	perror("fork failed");
}
