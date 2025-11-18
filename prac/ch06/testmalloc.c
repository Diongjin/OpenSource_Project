/*
* 6장 기본 함수
* 파일이름: testmalloc.c
*/

#include <stdio.h>
#include <stdlib.h>

int main()
{
	int max, i;
	int *ptr;

	printf("배열의 원소 개수는? ");
	scanf("%d",&max);

	/* 사용자가 입력한 수만큼 메모리 할당 */
	ptr = (int *)malloc(max * sizeof(int));

	/* 메모리 할당에 실패할 경우 */
	if (ptr == NULL){
		printf("메모리를 할당하지 못했습니다.");
		exit(-1);
	}

	for( i = 0; i < max; i++ )
		scanf("%d", &ptr[i]);

	printf("입력 숫자 리스트:");
	for( i = 0; i < max; i++ )
		printf("%d ", *(ptr+i));

	printf("\n");

	free((int *)ptr);
}
