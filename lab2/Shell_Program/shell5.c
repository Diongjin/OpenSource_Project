/* shell.c :
 * 1. exit
 * 2. 백그라운드 (&)
 * 3. SIGINT(Ctrl-C), SIGTSTP(Ctrl-Z)
 * 4. 리다이렉션(<, >), 파이프(|)
 * 5. 직접 구현: ls, pwd, cd, mkdir, rmdir, ln, cp, rm, mv, cat, grep
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>

int getargs(char *cmd, char **argv);

/* ======== 시그널 핸들러 (쉘용) ======== */
void sigint_handler(int signo) {
    write(STDOUT_FILENO, "\n(shell) SIGINT\nshell> ", 23);
}

void sigtstp_handler(int signo) {
    write(STDOUT_FILENO, "\n(shell) SIGTSTP\nshell> ", 24);
}

/* ======== 내장 명령 (부모에서 처리할 것들) ======== */
/* cd: 부모의 현재 디렉터리를 바꿔야 해서 부모에서 처리 */
int builtin_cd(char **argv) {
    if (argv[1] == NULL) {
        fprintf(stderr, "cd: path required\n");
        return -1;
    }
    if (chdir(argv[1]) < 0) {
        perror("cd");
        return -1;
    }
    return 0;
}

/* ======== 우리가 직접 구현하는 명령들 (자식에서 실행) ======== */

/* pwd */
int shell_pwd(char **argv) {
    char buf[4096];
    if (getcwd(buf, sizeof(buf)) == NULL) {
        perror("pwd");
        return 1;
    }
    printf("%s\n", buf);
    return 0;
}

/* ls: 옵션 없이 단순 목록, 인자 없으면 현재 디렉터리 */
int shell_ls(char **argv) {
    const char *path = ".";
    if (argv[1] != NULL) path = argv[1];

    DIR *dir = opendir(path);
    if (!dir) {
        perror("ls");
        return 1;
    }

    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        /* . .. 은 빼고 출력 */
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
            continue;
        printf("%s  ", ent->d_name);
    }
    printf("\n");
    closedir(dir);
    return 0;
}

/* mkdir: mkdir dir */
int shell_mkdir(char **argv) {
    if (argv[1] == NULL) {
        fprintf(stderr, "mkdir: path required\n");
        return 1;
    }
    if (mkdir(argv[1], 0755) < 0) {
        perror("mkdir");
        return 1;
    }
    return 0;
}

/* rmdir: rmdir dir (비어있는 디렉터리만 삭제) */
int shell_rmdir(char **argv) {
    if (argv[1] == NULL) {
        fprintf(stderr, "rmdir: path required\n");
        return 1;
    }
    if (rmdir(argv[1]) < 0) {
        perror("rmdir");
        return 1;
    }
    return 0;
}

/* ln: ln old new (하드 링크) */
int shell_ln(char **argv) {
    if (argv[1] == NULL || argv[2] == NULL) {
        fprintf(stderr, "ln: usage: ln old new\n");
        return 1;
    }
    if (link(argv[1], argv[2]) < 0) {
        perror("ln");
        return 1;
    }
    return 0;
}

/* cp: cp src dst (파일 -> 파일) */
int shell_cp(char **argv) {
    if (argv[1] == NULL || argv[2] == NULL) {
        fprintf(stderr, "cp: usage: cp src dst\n");
        return 1;
    }

    int fd_in  = open(argv[1], O_RDONLY);
    if (fd_in < 0) {
        perror("cp: open src");
        return 1;
    }

    int fd_out = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd_out < 0) {
        perror("cp: open dst");
        close(fd_in);
        return 1;
    }

    char buf[4096];
    ssize_t n;
    while ((n = read(fd_in, buf, sizeof(buf))) > 0) {
        if (write(fd_out, buf, n) != n) {
            perror("cp: write");
            close(fd_in);
            close(fd_out);
            return 1;
        }
    }
    if (n < 0) {
        perror("cp: read");
        close(fd_in);
        close(fd_out);
        return 1;
    }

    close(fd_in);
    close(fd_out);
    return 0;
}

/* rm: rm file (파일 삭제) */
int shell_rm(char **argv) {
    if (argv[1] == NULL) {
        fprintf(stderr, "rm: path required\n");
        return 1;
    }
    if (unlink(argv[1]) < 0) {
        perror("rm");
        return 1;
    }
    return 0;
}

/* mv: mv src dst (rename 기반) */
int shell_mv(char **argv) {
    if (argv[1] == NULL || argv[2] == NULL) {
        fprintf(stderr, "mv: usage: mv src dst\n");
        return 1;
    }
    if (rename(argv[1], argv[2]) < 0) {
        perror("mv");
        return 1;
    }
    return 0;
}

/* cat: cat [file] (없으면 stdin) */
int shell_cat(char **argv) {
    int fd;
    if (argv[1] == NULL) {
        fd = STDIN_FILENO;   // 인자 없으면 표준 입력 사용
    } else {
        fd = open(argv[1], O_RDONLY);
        if (fd < 0) {
            perror("cat");
            return 1;
        }
    }

    char buf[4096];
    ssize_t n;
    while ((n = read(fd, buf, sizeof(buf))) > 0) {
        if (write(STDOUT_FILENO, buf, n) != n) {
            perror("cat: write");
            if (fd != STDIN_FILENO) close(fd);
            return 1;
        }
    }
    if (n < 0) {
        perror("cat: read");
        if (fd != STDIN_FILENO) close(fd);
        return 1;
    }

    if (fd != STDIN_FILENO) close(fd);
    return 0;
}

/* grep: grep pattern [file] (단순 문자열 검색) */
int shell_grep(char **argv) {
    if (argv[1] == NULL) {
        fprintf(stderr, "grep: usage: grep pattern [file]\n");
        return 1;
    }

    const char *pattern = argv[1];
    FILE *fp = stdin;

    if (argv[2] != NULL) {
        fp = fopen(argv[2], "r");
        if (!fp) {
            perror("grep");
            return 1;
        }
    }

    char line[4096];
    while (fgets(line, sizeof(line), fp) != NULL) {
        if (strstr(line, pattern) != NULL) {
            fputs(line, stdout);
        }
    }

    if (fp != stdin) fclose(fp);
    return 0;
}

/* 우리가 구현한 명령인지 확인하고 실행.
 * 자식 프로세스에서 호출됨.
 * 실행했다면 1, 아니면 0 리턴.
 */
int run_my_command(char **argv) {
    if (argv[0] == NULL) return 0;

    if (strcmp(argv[0], "pwd")   == 0) { 
        shell_pwd(argv);   
        return 1; }
    if (strcmp(argv[0], "ls")    == 0) {
         shell_ls(argv);    
         return 1; }
    if (strcmp(argv[0], "mkdir") == 0) { 
        shell_mkdir(argv); 
        return 1; }
    if (strcmp(argv[0], "rmdir") == 0) { 
        shell_rmdir(argv); 
        return 1; }
    if (strcmp(argv[0], "ln")    == 0) { 
        shell_ln(argv);    
        return 1; }
    if (strcmp(argv[0], "cp")    == 0) { 
        shell_cp(argv);    
        return 1; }
    if (strcmp(argv[0], "rm")    == 0) { 
        shell_rm(argv);    
        return 1; }
    if (strcmp(argv[0], "mv")    == 0) { 
        shell_mv(argv);    
        return 1; }
    if (strcmp(argv[0], "cat")   == 0) { 
        shell_cat(argv);   
        return 1; }
    if (strcmp(argv[0], "grep")  == 0) { 
        shell_grep(argv);  
        return 1; }
    /* cd는 부모에서 처리 */

    return 0;   // 우리가 만든 명령이 아니면 0
}

/* ======== 메인 쉘 루프 ======== */
int main(void)
{
    char buf[256];
    char *argv[50];
    int narg;

    /* 3번: 인터럽트 키 처리 (쉘 프로세스) */
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
        buf[strcspn(buf, "\n")] = '\0';

        narg = getargs(buf, argv);
        if (narg == 0) continue;

        /* 1번: exit */
        if (strcmp(argv[0], "exit") == 0) {
            break;
        }

        /* cd는 쉘에서 직접 처리 (부모) */
        if (strcmp(argv[0], "cd") == 0) {
            builtin_cd(argv);
            continue;
        }

        /* 2번: 백그라운드 체크 (&) */
        int background = 0;
        if (strcmp(argv[narg - 1], "&") == 0) {
            background = 1;
            argv[narg - 1] = NULL;
            narg--;
        }

        /* 4번: 리다이렉션 & 파이프 분석 */
        int pipe_pos = -1;
        char *input_file = NULL;
        char *output_file = NULL;

        for (int i = 0; i < narg; i++) {
            if (!argv[i]) continue;
            if (strcmp(argv[i], "|") == 0) {
                pipe_pos = i;
                argv[i] = NULL;
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

        /* ---------- 파이프 없는 경우 ---------- */
        if (pipe_pos < 0) {
            pid_t pid = fork();
            if (pid == 0) {
                /* 자식: 시그널 기본 동작 */
                signal(SIGINT,  SIG_DFL);
                signal(SIGTSTP, SIG_DFL);

                /* 리다이렉션 */
                if (input_file) {
                    int fd = open(input_file, O_RDONLY);
                    if (fd < 0) { perror("open input"); exit(1); }
                    dup2(fd, STDIN_FILENO);
                    close(fd);
                }
                if (output_file) {
                    int fd = open(output_file,
                                  O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    if (fd < 0) { perror("open output"); exit(1); }
                    dup2(fd, STDOUT_FILENO);
                    close(fd);
                }

                /* 5번: 우리가 구현한 명령이면 여기서 처리 */
                if (run_my_command(argv)) {
                    exit(0);
                }

                /* 아니면 기존처럼 execvp */
                execvp(argv[0], argv);
                perror("execvp");
                exit(1);
            } else if (pid > 0) {
                if (!background) {
                    waitpid(pid, NULL, 0);
                } else {
                    printf("[bg pid=%d]\n", pid);
                }
            } else {
                perror("fork failed");
            }
        }
        /* ---------- 파이프 있는 경우: 왼쪽 | 오른쪽 ---------- */
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
                signal(SIGINT,  SIG_DFL);
                signal(SIGTSTP, SIG_DFL);

                if (input_file) {
                    int fd_in = open(input_file, O_RDONLY);
                    if (fd_in < 0) { perror("open input"); exit(1); }
                    dup2(fd_in, STDIN_FILENO);
                    close(fd_in);
                }

                dup2(fd[1], STDOUT_FILENO);
                close(fd[0]);
                close(fd[1]);

                if (run_my_command(left)) exit(0);

                execvp(left[0], left);
                perror("execvp left");
                exit(1);
            }

            pid_t pid2 = fork();
            if (pid2 == 0) {
                signal(SIGINT,  SIG_DFL);
                signal(SIGTSTP, SIG_DFL);

                if (output_file) {
                    int fd_out = open(output_file,
                                      O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    if (fd_out < 0) { perror("open output"); exit(1); }
                    dup2(fd_out, STDOUT_FILENO);
                    close(fd_out);
                }

                dup2(fd[0], STDIN_FILENO);
                close(fd[0]);
                close(fd[1]);

                if (run_my_command(right)) exit(0);

                execvp(right[0], right);
                perror("execvp right");
                exit(1);
            }

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

/* 공백/탭으로 자르는 간단 parser */
int getargs(char *cmd, char **argv)
{
    int narg = 0;
    while (*cmd) {
        if (*cmd == ' ' || *cmd == '\t')
            *cmd++ = '\0';
        else {
            argv[narg++] = cmd++;
            while (*cmd && *cmd != ' ' && *cmd != '\t')
                cmd++;
        }
    }
    argv[narg] = NULL;
    return narg;
}
