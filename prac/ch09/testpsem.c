#include <sys/types.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#define SEMNAME "/my_sem"
#include <unistd.h>

void testpsem(sem_t *semd);
void p(sem_t *semd);
void v(sem_t *semd);
int main()
{

	sem_t *semd;

	int i, val;

	if ((semd = sem_open(SEMNAME, O_CREAT, 0600, 1)) ==SEM_FAILED) {

		perror ("sem_open failed");

		exit (1);

	}

	sem_getvalue(semd, &val);

	printf("semaphore is %d\n", val);

	for (i = 0; i < 3; i++) {

		if (!fork())

			testpsem(semd);

	}

	sleep(10);

	if (sem_close(semd) == -1) {

		perror ("sem_close failed");

		exit (1);

	}

	if (sem_unlink(SEMNAME) == -1) {

		perror ("sem_unlink failed");

		exit (1);

	}

}


void testpsem (sem_t *semd)
{

	srand((unsigned int) getpid());

	p(semd);

	printf("process %d : semaphore in use\n", getpid());
	
	sleep(rand()%5);

	printf("process %d : putting semaphore\n",getpid());

	v(semd);

	exit(0);

}

void p (sem_t *semd)

{

	int ret;

	if ((ret = sem_trywait(semd)) != 0 && errno ==EAGAIN)

		sem_wait(semd);

	else if (ret != 0) {

		perror ("sem_trywait failed");

		exit (1);

	
	}
}

void v (sem_t *semd)

{

	if (sem_post(semd) != 0) {

		perror ("sem_post failed");

		exit (1);

	}

}
