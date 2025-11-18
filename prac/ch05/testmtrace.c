/*
5장 디버깅과 오류 처리
 *파일 이름 : testmtrace.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <mcheck.h>

#define MAX 20

int main(){
	int i;
	int *ptr1, *ptr2;

	mtrace();
	ptr1 = (int *)malloc(MAX * sizeof(int));
	ptr2 = (int *)malloc(MAX * sizeof(int));

	for (i = 0;i<MAX;i++){
		ptr1[i] = i;
		ptr2[i]=MAX - i;
	}
	free(ptr1);
	muntrace();
}
