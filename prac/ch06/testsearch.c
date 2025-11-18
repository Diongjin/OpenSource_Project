/*
* 6장 기본 함수
* 파일이름: testhsearch.c
*/

#include <stdio.h>
#include <search.h>
#include <string.h>

struct info {
	int id, age;
};
#define TABLESIZE 50

char nametable[TABLESIZE*20];
struct info infotable[TABLESIZE];

int main( )
{
char *nameptr = nametable; /* 이름테이블에서 다음 이름 */
struct info *infoptr = infotable; /* info테이블에서 다음 info */
	ENTRY item, *found;
	char name[30];
	int i = 0;
	/* 해쉬 테이블 생성 */
	(void) hcreate(TABLESIZE);
	while (scanf("%s%d%d", nameptr, &infoptr->id, &infoptr->age) != EOF && i++ < TABLESIZE) {
		item.key = nameptr;
		item.data = (char *)infoptr;
		
		/* 해쉬 테이블에 넣기 */

		(void) hsearch(item, ENTER);

		nameptr += strlen(nameptr) + 1;
		infoptr++;
	}
	clearerr(stdin); // 표준입력 클리어
	/* 해쉬 테이블 탐색 */
	item.key = name;
	while (scanf("%s", item.key) != EOF) {
		if ((found = hsearch(item, FIND)) != NULL) {
			printf("found %s, id=%d, age=%d\n",
				found->key,
				((struct info *)found->data)->id,
				((struct info *)found->data)->age);
		} else {
			printf("no such employee %s\n", name);
		}
	}
}
