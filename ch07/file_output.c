#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
	int fd; /* file descriptor */
	char *buf = "This is a test output file.\n";
	int flags = O_WRONLY | O_CREAT | O_TRUNC;
	mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP |
		S_IROTH; /* == 0644 */
	if (argc < 2) {
		fprintf(stderr, "Usage: file_output filename\n");
		exit(1);
	}
	if ( (fd = open(argv[1], flags, mode)) == -1 ) {
		perror("open"); /* errno에 대응하는 메시지 출력됨*/
		exit(1);
	}
	if ( dup2(fd, 1) == -1 ) {
		perror("dup2"); /* errno에 대응하는 메시지 출력됨 */
		exit(1);
	}
	if ( close(fd) == -1 ) {
		perror("close"); /* errno에 대응하는 메시지 출력됨*/
		exit(1);
	}
	write(1, buf, strlen(buf));
	exit(0);
}
