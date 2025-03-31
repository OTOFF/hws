#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <x86intrin.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define SHM_NAME "/covert_channel"  // 共享内存名称
#define BIT_INTERVAL 100000         // 每个比特的时钟周期
#define FLUSH_COUNT 10              // 刷新缓存的次数

volatile char *shared_mem;

void send_bit(bool bit) {
    uint64_t start = __rdtsc();
    while ((__rdtsc() - start) < BIT_INTERVAL) {
        if (bit) {
            // 发送 1：刷新缓存行
            for (int i = 0; i < FLUSH_COUNT; i++) {
                _mm_clflush((void *)shared_mem);
            }
        } else {
            // 发送 0：正常访问（填充缓存）
            (void)*shared_mem;
        }
    }
}

int main() {
    // 创建共享内存（跨用户可访问）
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, 4096);
    shared_mem = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    printf("[Sender] Shared Memory: %p\n", shared_mem);

    const char *msg = "SECRET";
    for (int i = 0; msg[i]; i++) {
        for (int j = 7; j >= 0; j--) {
            bool bit = (msg[i] >> j) & 1;
            send_bit(bit);
        }
    }

    munmap((void *)shared_mem, 4096);
    shm_unlink(SHM_NAME);
    return 0;
}