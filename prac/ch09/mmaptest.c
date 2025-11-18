#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/file.h>
#include <sys/wait.h>
#define FILE_SIZE 4096

int main(int argc, char **argv) {

	int fd, pid;

	/* open  * file */

	fd = open("testmmap", O_CREAT | O_RDWR| O_TRUNC, 0644);

	ftruncate(fd, FILE_SIZE);

	/* fork: 부모=writer, 자식=reader(공통FD) */

	pid = fork();

	if (pid < 0) { perror("fork"); exit(1); }

	/* 각자 주소공간에 mmap(MAP_SHARED) */

	void *addr = mmap(NULL, FILE_SIZE,PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	/* 공유 메모리의 8바이트를 카운터로 사용 */

	volatile uint64_t *counter = (volatile uint64_t*)addr;

	if (pid == 0) {

		/* ----- 자식: reader ----- */

		printf("[child ] started\n");

		uint64_t last = (uint64_t)-1;

		for (;;) {

			/* 공유 락으로 읽기 직렬화 */

			flock(fd, LOCK_SH);

			uint64_t v = *counter;

			flock(fd, LOCK_UN);

			if (v != last) {

				printf("[child ] observed counter= %llu\n", (unsigned long long)v);

				last = v;

				if (v >= 50) break;

			}
			munmap((void *)addr, FILE_SIZE);

			close(fd);

			printf("[child ] done\n");

			exit(0);

		} else {

			/* ----- 부모: writer ----- */

			printf("[parent] started\n");

			*counter = 0;

			for (uint64_t i = 0; i <= 50; i++) {

				/* 배타 락으로 쓰기 직렬화 */

				flock(fd, LOCK_EX);

				*counter = i;

				flock(fd, LOCK_UN);

				if (i % 5 == 0) {

					printf("[parent] wrote counter= %llu\n", (unsigned long long)i);

				}

				usleep(200 * 1000); /* 200ms */

			}

			/* child wait */

			int status = 0;

			if (waitpid(pid, &status, 0) < 0) {

				perror("waitpid");

				exit(1);

			}

			munmap((void *)addr, FILE_SIZE);

			close(fd);

			printf("[parent] done\n");

		}

		return 0;
}
