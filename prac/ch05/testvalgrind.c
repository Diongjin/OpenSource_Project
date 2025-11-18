/*
 * 5장 디버깅과 오류 처리
 * 파일 이름 : testvalgrind.c
 */
 
#include <stdlib.h>
#include <stdlib.h>
 
#define MAX 20
 
void sub(void)
{
	int i;
	int *ptr; 

	i = *ptr; /* 초기화되지 않은 메모리 사용 */
	ptr= malloc(MAX * sizeof(int));
	ptr[MAX] = 0; /* 불법적인 메모리 사용 */
	ptr = 0; /* 해제되지않은 메모리에 의한 메모리 누수 */
}


int main()
{
	sub();
}
