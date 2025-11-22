#include <stdio.h>
#include <stdlib.h>

extern int my_sum(int k); // cf. dbg_file1.c

int main(int argc, char *argv[])
{
	int i;
	int k;
	int sum = 0;

	if (argc < 2) {
		fprintf(stderr, "Usage: dbg_main 3\n");
		exit(1);
	}
	k=atoi(argv[1]);
	sum = my_sum(k);
	
	printf("sum = %d\n", sum);
} 
