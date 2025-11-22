#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/wait.h>

int main(void) {
    const char *filename = "lockfile.txt";
    char buf[128];
    int len;
    /* open file */
    int fd = open(filename, O_CREAT | O_RDWR, 0644);
    pid_t pid = fork();
    if (pid == 0) {
        /* --- 자식 프로세스 --- */
        printf("[child ] started\n");
        for (int i = 0; i < 5; i++) {
            flock(fd, LOCK_EX);
            printf("[child ] got lock, writing line %d\n", i);
            len = sprintf(buf, "child wrote line %d\n", i);
            write(fd, buf, len);
            sleep(1);
            flock(fd, LOCK_UN);
            printf("[child ] released lock\n");
            sleep(1);
        }
        close(fd);
        printf("[child ] done\n");
    } else {
        /* --- 부모 프로세스 --- */
        printf("[parent] started\n");
        for (int i = 0; i < 5; i++) {
            flock(fd, LOCK_EX);
            printf("[parent] got lock, writing line %d\n", i);
            len = sprintf(buf, "parent wrote line %d\n", i);
            write(fd, buf, len);
            sleep(1);
            flock(fd, LOCK_UN);
            printf("[parent] released lock\n");
            sleep(1);
        }
        /* wait child */
        int status;
        waitpid(pid, &status, 0);
        close(fd);
        printf("[parent] done\n");
    }
    return 0;
}