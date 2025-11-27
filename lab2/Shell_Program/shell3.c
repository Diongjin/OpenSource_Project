
/* shell.c : exit + 백그라운드(&) + SIGINT(Ctrl-C), SIGTSTP(Ctrl-Z) 처리 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>

int getargs(char *cmd, char **argv);   // getargs 프로토타입

/* --- 쉘에서 사용할 시그널 핸들러 (원하는 대로 간단 처리) --- */
void sigint_handler(int signo) {
    /* Ctrl-C 눌렀을 때 쉘은 안 죽고, 줄만 바꾸고 프롬프트 다시 출력 */
    write(STDOUT_FILENO, "\n(shell) SIGINT\nshell> ", 23);
}

void sigtstp_handler(int signo) {
    /* Ctrl-Z 눌렀을 때도 쉘은 안 죽음, 메시지 + 프롬프트 출력 */
    write(STDOUT_FILENO, "\n(shell) SIGTSTP\nshell> ", 24);
}

int main(void)
{
    char buf[256];
    char *argv[50];
    int narg;
    pid_t pid;

    /* 3번: 인터럽트 키 처리
       - 쉘 프로세스에서 SIGINT, SIGTSTP에 대한 동작 설정 */
    signal(SIGINT,  sigint_handler);   // Ctrl-C
    signal(SIGTSTP, sigtstp_handler);  // Ctrl-Z

    while (1) {
        printf("shell> ");
        fflush(stdout);

        if (fgets(buf, sizeof(buf), stdin) == NULL) {
            printf("\n");
            break;
        }
        clearerr(stdin);
        buf[strcspn(buf, "\n")] = '\0';   // 개행(\n) 제거

        narg = getargs(buf, argv);
        if (narg == 0) continue;

        /* 1번: exit 입력 시 쉘 종료 */
        if (strcmp(argv[0], "exit") == 0) {
            break;
        }

        /* 2번: 백그라운드(&) 여부 확인 */
        int background = 0;
        if (strcmp(argv[narg - 1], "&") == 0) {
            background = 1;
            argv[narg - 1] = NULL;   // "&" 토큰 제거
        }

        pid = fork();
        if (pid == 0) {
            /* ===== 자식 프로세스 영역 ===== */

            /* 부모(쉘)에서 설정한 시그널 처리 방식을
               자식에서는 "기본 동작"으로 돌려놓음 */
            signal(SIGINT,  SIG_DFL);   // Ctrl-C: 프로그램 종료
            signal(SIGTSTP, SIG_DFL);   // Ctrl-Z: 일시중지(stop)

            execvp(argv[0], argv);
            perror("execvp");
            exit(1);
        }
        else if (pid > 0) {
            /* ===== 부모(쉘) 프로세스 ===== */
            if (!background) {
                /* 포그라운드 실행: 자식이 끝날 때까지 대기 */
                waitpid(pid, NULL, 0);
            } else {
                /* 백그라운드 실행: 그냥 PID만 찍고 기다리지 않음 */
                printf("[bg pid=%d]\n", pid);
            }
        }
        else {
            perror("fork failed");
        }
    }

    return 0;
}

/* 기존 getargs 그대로 사용 */
int getargs(char *cmd, char **argv)
{
    int narg = 0;
    while (*cmd) {
        if (*cmd == ' ' || *cmd == '\t')
            *cmd++ = '\0';
        else {
            argv[narg++] = cmd++;
            while (*cmd != '\0' && *cmd != ' ' && *cmd != '\t')
                cmd++;
        }
    }
    argv[narg] = NULL;
    return narg;
}
