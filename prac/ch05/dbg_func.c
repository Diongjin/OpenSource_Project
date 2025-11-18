#include <stdio.h>
#include <stdlib.h>

int my_sum(int k)
{
	int i;
	int sum = 0;
	for (i=1; i<=k; i++)
		sum += i;
	return sum;
}

int main(int argc, char *argv[])
{
	int i;
	int k;
	int sum = 0;

	if (argc < 2) {
		fprintf(stderr, "Usage: dbg_func 3\n");
		exit(1);
	}
	
	k = atoi(argv[1]);
	sum = my_sum(k);
	printf("sum = %d\n", sum);
} 
