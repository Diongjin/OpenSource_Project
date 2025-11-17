/*
* 7장 파일 처리
* 파일 이름: file_size.c
*/
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
int main(int argc, char *argv[])
{
	int fd; /* file descriptor */
	off_t size;

	if (argc < 2) {
		fprintf(stderr, "Usage: file_size filename\n");
		exit(1);
	}

	if ( (fd = open(argv[1], O_RDONLY)) == -1 ){
		perror("open"); /* errno에 대응하는 메시지 출력됨 */
		exit(1);
	}

	size = lseek(fd, 0, SEEK_END);

	if (size < 0) {
		perror("lseek");
		exit(1);
	}

	printf("%s\'s size = %ld\n", argv[1], size);
	close(fd);
}
