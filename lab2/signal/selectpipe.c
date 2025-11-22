#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#define MSGSIZE 16

char *hello_msg = "Hello, parent!";
char *bye_msg = "Bye, parent!";
void parent(int [3][2]);
void child(int [2]);

int main()
{
    int pp[3][2], i;
    int pid;
    /* open pipe */
    for (i = 0; i < 3; i++) {
        if (pipe(pp[i]) == -1) {
            perror ("pipe call failed");
            exit (1);
        }
        pid = fork();
        if (pid == 0) /* child process */
        child (pp[i]);
        else if (pid == -1) {
            perror ("fork failed");
            exit (1);
        }
    }
    /* parent process */
    parent (pp);
}
void parent(int pp[3][2])
{
    char buf[MSGSIZE], ch;
    fd_set set, master;
    int i;
    for (i = 0; i < 3; i++)
    close(pp[i][1]);
    /* set bit mask of select system call */
    FD_ZERO (&master);
    FD_SET (0, &master);
    for (i = 0; i < 3; i++)
    FD_SET (pp[i][0], &master);
    while (set=master, select (pp[2][0]+1,
        &set, NULL, NULL, NULL) > 0) {
            if (FD_ISSET(0, &set)) {
                printf("From standard input: \n");
                read (0, &ch, 1);
                printf("%c\n", ch);
            }
            for (i = 0; i < 3; i++) {
                if (FD_ISSET(pp[i][0], &set))
                if (read(pp[i][0], buf, MSGSIZE) > 0)
                printf("message is %s fromchild %d\n", buf, i);
                }
                if (waitpid (-1, NULL, WNOHANG) == -1)
                return;
            }
        }
        void child(int p[2])
        {
            int j;
            /* write to pipe */
            close (p[0]);
            for (j = 0; j < 3; j++) {
                write(p[1], hello_msg, MSGSIZE);
                sleep (getpid() % 4);
            }
            write(p[1], bye_msg, MSGSIZE);
            exit (0);
}