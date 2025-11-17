#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{

	int count;
	int i, seed, n;
	double x, y, pi;

	if (argc < 3) {
		printf("calcpy <#samples> <seed>\n");
		exit(0);
	}

	n = atoi(argv[1]);
	seed = atoi(argv[2]);
	count = 0;
	srand(seed);
	for (i=0 ; i < n; i++) {
		x = rand()/(RAND_MAX+1.0);
		y = rand()/(RAND_MAX+1.0);
		if (x*x + y*y <= 1.0) count++;
	}
	pi = 4.0 * count / n;
	printf("Samples: %d Pi: %7.5f\n", n, pi);
}
