#include <stdio.h>
#include "calc.h"

int main(){

	int a = 100, b=10;

	printf("add : %d\n",add(a,b));
	printf("subtract : %d\n", subtract(a,b));
	printf("multiply : %d\n", multiply(a,b));
	printf("divide : %d\n", divide(a,b));
	
	return 0;
}
			
