#include <stdio.h>
#include <string.h>
#include <time.h>

#define MAX_LEN 256
#define NUM_SENTENCES 4

int main(void){
const char *sentences[NUM_SENTENCES] = {
        "동해물과 백두산이 마르고 닳도록",
        "하느님이 보우하사 우리나라 만세",
        "무궁화 삼천리 화려강산",
        "대한사람 대한으로 우리나라 만세"
    };

    char input[MAX_LEN];
    int total_errors = 0;   // 오타
    int total_chars = 0;    // 
    double total_time = 0.0;

    printf("=== 타자 연습 프로그램 (애국가 1절) ===\n\n");

    for (int i = 0; i < NUM_SENTENCES; i++) {
        printf("[%d/%d] 다음 문장을 그대로 입력하세요.\n", i + 1, NUM_SENTENCES);
        printf("문장: %s\n", sentences[i]);
        printf("입력: ");

        // 시간 측정 시작
        time_t start = time(NULL);

        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("입력 오류\n");
            return 1;
        }

        // 시간 측정 끝
        time_t end = time(NULL);
        double elapsed = difftime(end, start); // 초 단위 경과 시간

        // 개행 문자 제거
        input[strcspn(input, "\n")] = '\0';

        size_t len_target = strlen(sentences[i]);
        size_t len_input  = strlen(input);
        size_t maxlen = (len_target > len_input) ? len_target : len_input;

        int errors = 0;
        for (size_t j = 0; j < maxlen; j++) {
            char c1 = (j < len_target) ? sentences[i][j] : '\0';
            char c2 = (j < len_input)  ? input[j]         : '\0';
            if (c1 != c2) {
                errors++;
            }
        }

        printf("→ 이 문장에서 틀린 글자 수: %d\n\n", errors);

        total_errors += errors;
        total_chars  += (int)len_target;  // 기준: 제시한 문장 길이 기준
        total_time   += elapsed;
    }

    if (total_time <= 0.0) {
        total_time = 1.0; // 0으로 나누는 것 방지용
    }

    double minutes = total_time / 60.0;
    double cpm = total_chars / minutes;    // characters per minute
    double wpm = cpm / 5.0;               // 보통 5타 = 1단어로 가정

    printf("=== 결과 ===\n");
    printf("총 틀린 글자 수: %d\n", total_errors);
    printf("총 입력한 기준 글자 수: %d\n", total_chars);
    printf("총 소요 시간: %.1f초\n", total_time);
    printf("평균 분당 타수(타/분): %.1f\n", cpm);
    printf("평균 분당 타수(단어/분, 5타=1단어 가정): %.1f\n", wpm);

    return 0;

}