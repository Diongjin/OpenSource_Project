#include <stdio.h>
#include <dlfcn.h>

int main()
{
	void *handle;
	int (*max)(int, int), (*min)(int,int);
	char *error;

	handle = dlopen ("./lb/libtest.so",RTLD_LAZY);
	if(!handle) {
		fputs (dlerror(),stderr);
		exit(1);
	}

	max = dlsym(handle,"max");

	if((error = dlerror()) != NULL) {
		fprintf (stderr, "%s", error);
			exit(1);
	}
	min = dlsym(handle, "min");

	if((error =dlerror()) !=NULL) {
		fprintf (stderr, "%s", error);
		exit(1);
	}

	printf("max(1,2) = %d\n", (*max)(1,2));
	printf("min(1,2) = %d\n", (*min)(1,2));
	dlclose(handle);
}
