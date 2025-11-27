
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>      // open()

int getargs(char *cmd, char **argv);   // getargs 프로토타입

/* --- 쉘에서 사용할 시그널 핸들러 --- */
void sigint_handler(int signo) {
    write(STDOUT_FILENO, "\n(shell) SIGINT\nshell> ", 23);
}

void sigtstp_handler(int signo) {
    write(STDOUT_FILENO, "\n(shell) SIGTSTP\nshell> ", 24);
}

int main(void)
{
    char buf[256];
    char *argv[50];
    int narg;
    pid_t pid;

    /* 3번: 인터럽트 키 처리 (쉘 프로세스 쪽 처리) */
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
        buf[strcspn(buf, "\n")] = '\0';   // 개행 제거

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
            narg--;
        }

        /* 4번: 리다이렉션(<, >) + 파이프(|) 분석 */
        int pipe_pos = -1;
        char *input_file = NULL;
        char *output_file = NULL;

        for (int i = 0; i < narg; i++) {
            if (argv[i] == NULL) continue;

            if (strcmp(argv[i], "|") == 0) {
                pipe_pos = i;
                argv[i] = NULL;   // 왼쪽 명령과 오른쪽 명령을 나누기 위해
            } else if (strcmp(argv[i], "<") == 0) {
                if (i + 1 < narg) {
                    input_file = argv[i + 1];
                    argv[i] = NULL;
                    argv[i + 1] = NULL;
                }
            } else if (strcmp(argv[i], ">") == 0) {
                if (i + 1 < narg) {
                    output_file = argv[i + 1];
                    argv[i] = NULL;
                    argv[i + 1] = NULL;
                }
            }
        }

        /* ===== 파이프가 없는 경우: 단일 명령 실행 ===== */
        if (pipe_pos < 0) {
            pid = fork();
            if (pid == 0) {
                /* 자식: 시그널 기본 동작으로 되돌림 */
                signal(SIGINT,  SIG_DFL);
                signal(SIGTSTP, SIG_DFL);

                /* 입력 리다이렉션 */
                if (input_file != NULL) {
                    int fd = open(input_file, O_RDONLY);
                    if (fd < 0) {
                        perror("open input");
                        exit(1);
                    }
                    dup2(fd, STDIN_FILENO);
                    close(fd);
                }

                /* 출력 리다이렉션 */
                if (output_file != NULL) {
                    int fd = open(output_file,
                                  O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    if (fd < 0) {
                        perror("open output");
                        exit(1);
                    }
                    dup2(fd, STDOUT_FILENO);
                    close(fd);
                }

                execvp(argv[0], argv);
                perror("execvp");
                exit(1);
            }
            else if (pid > 0) {
                if (!background) {
                    waitpid(pid, NULL, 0);
                } else {
                    printf("[bg pid=%d]\n", pid);
                }
            }
            else {
                perror("fork failed");
            }
        }
        /* ===== 파이프가 있는 경우: cmd1 | cmd2 ===== */
        else {
            int fd[2];
            if (pipe(fd) < 0) {
                perror("pipe");
                continue;
            }

            char **left  = &argv[0];
            char **right = &argv[pipe_pos + 1];

            pid_t pid1 = fork();
            if (pid1 == 0) {
                /* 왼쪽 명령: stdout -> 파이프 write */
                signal(SIGINT,  SIG_DFL);
                signal(SIGTSTP, SIG_DFL);

                /* 왼쪽 명령에 대해서만 입력 리다이렉션 적용 */
                if (input_file != NULL) {
                    int fd_in = open(input_file, O_RDONLY);
                    if (fd_in < 0) { perror("open input"); exit(1); }
                    dup2(fd_in, STDIN_FILENO);
                    close(fd_in);
                }

                dup2(fd[1], STDOUT_FILENO);
                close(fd[0]);
                close(fd[1]);

                execvp(left[0], left);
                perror("execvp left");
                exit(1);
            }

            pid_t pid2 = fork();
            if (pid2 == 0) {
                /* 오른쪽 명령: stdin <- 파이프 read */
                signal(SIGINT,  SIG_DFL);
                signal(SIGTSTP, SIG_DFL);

                /* 오른쪽 명령에 대해서만 출력 리다이렉션 적용 */
                if (output_file != NULL) {
                    int fd_out = open(output_file,
                                      O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    if (fd_out < 0) { perror("open output"); exit(1); }
                    dup2(fd_out, STDOUT_FILENO);
                    close(fd_out);
                }

                dup2(fd[0], STDIN_FILENO);
                close(fd[0]);
                close(fd[1]);

                execvp(right[0], right);
                perror("execvp right");
                exit(1);
            }

            /* 부모: 파이프 닫기 */
            close(fd[0]);
            close(fd[1]);

            if (!background) {
                waitpid(pid1, NULL, 0);
                waitpid(pid2, NULL, 0);
            } else {
                printf("[bg pipe pids=%d,%d]\n", pid1, pid2);
            }
        }
    }

    return 0;
}

/* getargs: 공백/탭 기준으로 문자열을 잘라 argv 배열에 저장 */
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