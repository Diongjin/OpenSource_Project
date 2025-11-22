/*
 * 5장 디버깅과 오류 처리
 * 파일 이름: dbg_data.c */ 
 
#include <stdio.h> 

int add(int i, int j) 
{ 
	int k; 

	k = i + j; 
	return k; 
} 

int main() 
{ 
	int m = 2, n = 3; 
	int sum; 

	sum = m + n; 
	printf("sum = %d\n", sum); 

	sum = add(m, n); 
	printf("sum = %d", sum); 
} 
