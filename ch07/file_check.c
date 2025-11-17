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

	if (argc < 2) {
		fprintf(stderr, "Usage: file_check filename\n");
		exit(1);
	}

	if ( (fd = open(argv[1], O_RDONLY)) == -1 ) {
		perror("open"); /* errno에 대응하는 메시지출력됨 */
		exit(1);
	}

	printf("File \"%s\" found...\n", argv[1]);
	close(fd);
}


