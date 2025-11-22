// file_copy_shm.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>

#define SHM_NAME "/filecopy_shm"  // POSIX 공유 메모리 이름

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "사용법: %s <원본파일> <복사될파일>\n", argv[0]);
        return 1;
    }

    const char *src_path = argv[1];
    const char *dst_path = argv[2];

    int src_fd, dst_fd, shm_fd;
    struct stat st;
    size_t filesize;
    void *src_map = NULL;
    void *dst_map = NULL;
    void *shm_map = NULL;

    // 1. 원본 파일 열기
    src_fd = open(src_path, O_RDONLY);
    if (src_fd == -1) {
        perror("open src");
        return 1;
    }

    // 2. 파일 크기 구하기
    if (fstat(src_fd, &st) == -1) {
        perror("fstat");
        close(src_fd);
        return 1;
    }
    filesize = st.st_size;

    if (filesize == 0) {
        // 빈 파일이면 그냥 대상 파일만 만들어주고 끝
        dst_fd = open(dst_path, O_CREAT | O_TRUNC, 0644);
        if (dst_fd == -1) {
            perror("open dst");
            close(src_fd);
            return 1;
        }
        close(src_fd);
        close(dst_fd);
        printf("빈 파일 복사 완료.\n");
        return 0;
    }

    // 3. 대상 파일 열기/생성
    dst_fd = open(dst_path, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (dst_fd == -1) {
        perror("open dst");
        close(src_fd);
        return 1;
    }

    // 대상 파일 크기를 원본과 동일하게 설정
    if (ftruncate(dst_fd, filesize) == -1) {
        perror("ftruncate dst");
        close(src_fd);
        close(dst_fd);
        return 1;
    }

    // 4. 공유 메모리 객체 생성
    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        close(src_fd);
        close(dst_fd);
        return 1;
    }

    // 공유 메모리 크기를 파일 크기와 동일하게 설정
    if (ftruncate(shm_fd, filesize) == -1) {
        perror("ftruncate shm");
        close(src_fd);
        close(dst_fd);
        close(shm_fd);
        shm_unlink(SHM_NAME);
        return 1;
    }

    // 5. fork로 부모/자식 나누기
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        close(src_fd);
        close(dst_fd);
        close(shm_fd);
        shm_unlink(SHM_NAME);
        return 1;
    }

    if (pid == 0) {
        // -------------------- 자식 프로세스: 공유 메모리 -> 대상 파일 --------------------
        // 공유 메모리 매핑 (읽기 전용)
        shm_map = mmap(NULL, filesize, PROT_READ, MAP_SHARED, shm_fd, 0);
        if (shm_map == MAP_FAILED) {
            perror("mmap shm (child)");
            exit(1);
        }

        // 대상 파일 매핑 (읽기/쓰기)
        dst_map = mmap(NULL, filesize, PROT_READ | PROT_WRITE, MAP_SHARED, dst_fd, 0);
        if (dst_map == MAP_FAILED) {
            perror("mmap dst (child)");
            munmap(shm_map, filesize);
            exit(1);
        }

        // 공유 메모리 내용을 대상 파일로 복사
        memcpy(dst_map, shm_map, filesize);

        // 디스크에 동기화
        if (msync(dst_map, filesize, MS_SYNC) == -1) {
            perror("msync dst");
        }

        munmap(shm_map, filesize);
        munmap(dst_map, filesize);

        close(src_fd);
        close(dst_fd);
        close(shm_fd);

        exit(0);
    } else {
        // -------------------- 부모 프로세스: 원본 파일 -> 공유 메모리 --------------------
        // 원본 파일 매핑 (읽기 전용)
        src_map = mmap(NULL, filesize, PROT_READ, MAP_PRIVATE, src_fd, 0);
        if (src_map == MAP_FAILED) {
            perror("mmap src (parent)");
            close(src_fd);
            close(dst_fd);
            close(shm_fd);
            shm_unlink(SHM_NAME);
            return 1;
        }

        // 공유 메모리 매핑 (읽기/쓰기)
        shm_map = mmap(NULL, filesize, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
        if (shm_map == MAP_FAILED) {
            perror("mmap shm (parent)");
            munmap(src_map, filesize);
            close(src_fd);
            close(dst_fd);
            close(shm_fd);
            shm_unlink(SHM_NAME);
            return 1;
        }

        // 원본 파일 내용을 공유 메모리로 복사
        memcpy(shm_map, src_map, filesize);

        munmap(src_map, filesize);
        munmap(shm_map, filesize);

        close(src_fd);
        close(dst_fd);
        close(shm_fd);

        // 자식 종료 대기
        wait(NULL);

        // 공유 메모리 객체 제거
        shm_unlink(SHM_NAME);

        printf("파일 복사 완료: %s -> %s (크기: %ld bytes)\n",
               src_path, dst_path, (long)filesize);
    }

    return 0;
}
