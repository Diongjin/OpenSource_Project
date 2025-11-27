#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

int getargs(char *cmd, char **argv);   // getargs 프로토타입

int main(void)
{
    char buf[256];
    char *argv[50];
    int narg;
    pid_t pid;

    while (1) {
        printf("shell> ");
        fflush(stdout);

        if (fgets(buf, sizeof(buf), stdin) == NULL) {
            printf("\n");
            break;
        }
        clearerr(stdin);
        buf[strcspn(buf, "\n")] = '\0';

        narg = getargs(buf, argv);
        if (narg == 0) continue;

        /* exit → 쉘 종료 */
        if (strcmp(argv[0], "exit") == 0) {
            break;
        }

        /* -------------------------
           백그라운드 여부 체크
           - 마지막 토큰이 "&"면 백그라운드 실행
        ------------------------- */
        int background = 0;
        if (strcmp(argv[narg - 1], "&") == 0) {
            background = 1;
            argv[narg - 1] = NULL;   // "&" 제거
        }


        /* -------------------------
        fork 이후, 백그라운드면 기다리지 않는 부분
        ------------------------- */
        pid = fork();
        if (pid == 0) {
            /* 자식: 명령 실행 */
            execvp(argv[0], argv);
            perror("execvp");
            exit(1);
        }
        else if (pid > 0) {
            /* 부모 */
            if (!background) {
                /* 포그라운드 실행: 기다림 */
                waitpid(pid, NULL, 0);
            } else {
                /* 백그라운드 실행: 기다리지 않음 */
                printf("[bg pid=%d]\n", pid);
            }
        }
        else {
            perror("fork failed");
        }
    }

    return 0;
}

/* getargs: 공백으로 문자열 나누기 */
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