#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>


#define MSGSIZE 16

int main()
{
    char buf[MSGSIZE];
    int p[2], i;
    int pid;
    /* open pipe */
    if (pipe(p) == -1) {
        perror ("pipe call failed");
        exit(1);
    }
    pid = fork();
    if (pid == 0) { /* child process */
        close(p[0]);
        /* write to pipe */
        for (i = 0; i < 2; i++) {
            sprintf(buf, "Hello, world #%d",i+1);
                write(p[1], buf, MSGSIZE);
            }
        }
        else if (pid > 0) {
            close(p[1]);
            /* read from pipe */
            for (i = 0; i < 2; i++) {
                read (p[0], buf, MSGSIZE);
                printf ("%s\n", buf);
            }
        }
        else
            perror ("fork failed");
}