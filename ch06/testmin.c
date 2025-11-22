/*
* 6장 기본 함수
* 파일이름: testmin.c
*/

#include <stdio.h>
#include <string.h>
int main(void) /*문자열 처리*/
{	
	char s[20],t[20],min[20];
	
	printf("char 1 ? "); scanf("%s",s);
	printf("char 2 ? "); scanf("%s",t);

	if (strcmp(s,t)<0)
		strcpy(min,s);

	else
		strcpy(min,t);

	printf("%s\n", min);
}
