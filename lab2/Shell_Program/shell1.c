#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

int getargs(char *cmd, char **argv);

int main()
{
    char buf[256];
    char *argv[50];
    int narg;
    pid_t pid;

    while (1) {
        printf("shell> ");
        if (fgets(buf, sizeof(buf), stdin) == NULL) {
            // EOF(Ctrl+D 등) 들어오면 종료
            break;
        }

        // 윈도우/WSL에서 들어오는 \r\n 또는 리눅스의 \n 모두 제거
        buf[strcspn(buf, "\r\n")] = '\0';   // ← 수정 포인트

        clearerr(stdin);

        // 토큰 분리
        narg = getargs(buf, argv);
        if (narg == 0)
            continue;

        // 첫 번째 인자가 exit이면 셸 종료
        if (strcmp(argv[0], "exit") == 0)   // ← 수정 포인트
            break;

        // 자식 프로세스 생성 후 execvp 실행
        pid = fork();
        if (pid == 0) {
            execvp(argv[0], argv);
            // 여기까지 왔다는 건 execvp 실패
            perror("execvp failed");
            exit(1);
        } else if (pid > 0) {
            wait((int *)0);
        } else {
            perror("fork failed");
        }
    }

    return 0;
}

int getargs(char *cmd, char **argv)
{
    int narg = 0;

    while (*cmd) {
        // 공백/탭/캐리지리턴을 구분자로 사용
        if (*cmd == ' ' || *cmd == '\t' || *cmd == '\r')   // ← '\r' 추가 가능
            *cmd++ = '\0';
        else {
            argv[narg++] = cmd++;
            while (*cmd != '\0' &&
                   *cmd != ' ' &&
                   *cmd != '\t' &&
                   *cmd != '\r')                          // ← '\r' 추가 가능
                cmd++;
        }
    }
    argv[narg] = NULL;
    return narg;
}
