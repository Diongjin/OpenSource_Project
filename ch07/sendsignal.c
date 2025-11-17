#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

int i=0;
void p_handler (int), c_handler (int);

int main(){
	pid_t pid,ppid;
	struct sigaction act;

	pid = fork();
	if (pid == 0){
		act.sa_handler = c_handler;
		sigaction (SIGUSR1, &act, NULL);
		ppid = getppid();
		while (1){
			sleep(1);
			kill(ppid,SIGUSR1);
			pause();
		}
	}
	else if (pid>0){
		act.sa_handler = p_handler;
		sigaction(SIGUSR1, &act,NULL);
		while(1){
			pause();
			sleep(1);
			kill(pid,SIGUSR1);
		}
	}
	else
		perror("Error");
}

void p_handler(int signo)
{
	printf("Parent handler: call %d times.\n",++i);
}
void c_handler(int signo){
	printf("Child handler: call %d times.\n",++i);
}
