#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define MAX_BUF 256

int main(int argc, char *argv[])
{

    char buf[MAX_BUF];
    if (argc < 2) {
        fprintf(stderr, "Usage: file_chdir dirname\n");
        exit(1);
    }
    memset(buf, 0, MAX_BUF);
    if (getcwd(buf, MAX_BUF) < 0) {
        perror("getcwd");
        exit(1);
    }
    printf("working directory (before) = %s\n", buf);
    if (chdir(argv[1]) < 0) {
        perror("chdir");
        exit(1);
    }
    memset(buf, 0, MAX_BUF);
    if (getcwd(buf, MAX_BUF) < 0) {
        perror("getcwd");
        exit(1);
    }
    printf("working directory (after ) = %s\n", buf);
}