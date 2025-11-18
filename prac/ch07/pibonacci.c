#include <stdio.h>
#include <signal.h>
#include <setjmp.h>
#include <stdlib.h>
#include <sys/wait.h>
void start(int signo);
sigjmp_buf jbuf;

int main()
{
	struct sigaction act;
	int cur_i, past_i, tmp_i;
	/* 현재 위치를 저장 */
	if (sigsetjmp(jbuf, 1) == 0) {
		act.sa_handler = start;
		sigaction(SIGINT, &act, NULL);
	}

	cur_i = past_i = 1;
	while (1) {
		printf("%d\n", cur_i);
		tmp_i = cur_i;
		cur_i += past_i;
		past_i = tmp_i;
		sleep(1);
	}

}
void start(int signo)
{
	fprintf(stderr, "Interrupted\n");
	siglongjmp(jbuf, 1); /* 저장된 곳으로 복귀 */
}
