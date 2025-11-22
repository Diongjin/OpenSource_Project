/* shell.c : exit + 백그라운드(&) + SIGINT/SIGTSTP + 리다이렉션(<, >), 파이프(|)
 *          + 직접 구현: mv, cat, grep
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>      // open()
#include <sys/stat.h>   // rename, 파일 권한 등

int getargs(char *cmd, char **argv);   // getargs 프로토타입

/* --- 쉘에서 사용할 시그널 핸들러 --- */
void sigint_handler(int signo) {
    write(STDOUT_FILENO, "\n(shell) SIGINT\nshell> ", 23);
}

void sigtstp_handler(int signo) {
    write(STDOUT_FILENO, "\n(shell) SIGTSTP\nshell> ", 24);
}

/* =========================
   5번 과제: mv, cat, grep 구현
   (자식 프로세스에서 실행)
   ========================= */

/* mv: mv src dst  -> rename() 사용 */
int my_mv(char **argv) {
    if (argv[1] == NULL || argv[2] == NULL) {
        fprintf(stderr, "mv: src and dst required\n");
        return 1;
    }
    if (rename(argv[1], argv[2]) < 0) {
        perror("mv");
        return 1;
    }
    return 0;
}

/* cat: cat file1 [file2 ...]  (표준입력 cat은 생략) */
int my_cat(char **argv) {
    if (argv[1] == NULL) {
        fprintf(stderr, "cat: file required\n");
        return 1;
    }

    char buf[4096];
    for (int i = 1; argv[i] != NULL; i++) {
        int fd = open(argv[i], O_RDONLY);
        if (fd < 0) {
            perror("cat open");
            continue;   // 다음 파일 시도
        }

        ssize_t n;
        while ((n = read(fd, buf, sizeof(buf))) > 0) {
            if (write(STDOUT_FILENO, buf, n) != n) {
                perror("cat write");
                close(fd);
                return 1;
            }
        }
        if (n < 0) perror("cat read");
        close(fd);
    }
    return 0;
}

/* grep: grep pattern file   (아주 단순 버전) */
int my_grep(char **argv) {
    if (argv[1] == NULL || argv[2] == NULL) {
        fprintf(stderr, "grep: pattern and file required\n");
        return 1;
    }

    const char *pattern = argv[1];
    const char *filename = argv[2];

    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("grep fopen");
        return 1;
    }

    char line[4096];
    while (fgets(line, sizeof(line), fp) != NULL) {
        if (strstr(line, pattern) != NULL) {
            fputs(line, stdout);
        }
    }

    fclose(fp);
    return 0;
}

/* 우리가 구현한 명령인지 확인하고 실행
 * - 실행했다면 1, 아니면 0 리턴
 * - 자식 프로세스에서 호출됨
 */
int run_my_command(char **argv) {
    if (argv[0] == NULL) return 0;

    if (strcmp(argv[0], "mv") == 0) {
        my_mv(argv);
        return 1;
    }
    if (strcmp(argv[0], "cat") == 0) {
        my_cat(argv);
        return 1;
    }
    if (strcmp(argv[0], "grep") == 0) {
        my_grep(argv);
        return 1;
    }

    return 0;   // 우리가 만든 명령이 아니면 0
}

/* =========================
   여기부터 기존 4번까지 코드 + run_my_command 호출 추가
   ========================= */

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

                /* 5번: mv/cat/grep이면 우리가 직접 처리 */
                if (run_my_command(argv)) {
                    exit(0);
                }

                /* 나머지는 기존처럼 execvp */
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

                /* 왼쪽에 mv/cat/grep이 올 수도 있으니 처리 */
                if (run_my_command(left)) exit(0);

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

                /* 오른쪽에 mv/cat/grep이 올 수도 있으니 처리 */
                if (run_my_command(right)) exit(0);

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