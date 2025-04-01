#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <x86intrin.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>

#define SHM_NAME "/covert_channel"
#define BIT_INTERVAL 100000
#define FLUSH_COUNT 10

volatile char *shared_mem;

void send_bit(bool bit) {
    uint64_t start = __rdtsc();
    while ((__rdtsc() - start) < BIT_INTERVAL) {
        if (bit) {
            for (int i = 0; i < FLUSH_COUNT; i++) {
                _mm_clflush((void *)shared_mem);
            }
        } else {
            (void)*shared_mem;
        }
    }
}

int main() {
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, 4096);
    shared_mem = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    printf("[Sender] Shared Memory: %p\n", shared_mem);

    while (1) {  // 无限循环，保持 sender 不退出
        const char *msg = "SECRET";
        for (int i = 0; msg[i]; i++) {
            for (int j = 7; j >= 0; j--) {
                bool bit = (msg[i] >> j) & 1;
                send_bit(bit);
            }
        }
        sleep(1);  // 每发送完一次后暂停 1 秒
    }

    // 以下代码不会执行（因为 while(1)）
    munmap((void *)shared_mem, 4096);
    shm_unlink(SHM_NAME);
    return 0;
}