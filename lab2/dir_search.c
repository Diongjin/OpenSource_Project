#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

void list_dir(const char *path, int depth) {
    DIR *dir;
    struct dirent *entry;
    struct stat info;

    // 현재 디렉토리 출력
    for (int i = 0; i < depth; i++) printf("  ");
    printf("%s:\n", path);

    dir = opendir(path);
    if (dir == NULL) {
        perror("opendir");
        return;
    }

    // 디렉토리 변경
    chdir(path);

    while ((entry = readdir(dir)) != NULL) {
        // . 과 .. 은 무시
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0)
            continue;

        // 파일 정보 읽기
        lstat(entry->d_name, &info);

        // 들여쓰기
        for (int i = 0; i < depth + 1; i++) printf("  ");
        printf("%s\n", entry->d_name);

        // 디렉토리면 재귀적으로 들어가기
        if (S_ISDIR(info.st_mode)) {
            char new_path[512];
            strcpy(new_path, entry->d_name);

            list_dir(new_path, depth + 1);
        }
    }

    // 디렉토리 원상복귀
    chdir("..");
    closedir(dir);
}

int main(int argc, char *argv[]) {
    char path[256];

    printf("탐색할 디렉토리 이름 입력 (예: .): ");
    scanf("%s", path);

    list_dir(path, 0);

    return 0;
}
