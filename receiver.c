#include <stdio.h>
#include <stdint.h>
#include <x86intrin.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define SHM_NAME "/covert_channel"
#define SAMPLE_WINDOW 100000
#define THRESHOLD 150  // 缓存命中/未命中的阈值

volatile char *shared_mem;

bool detect_bit() {
    uint64_t start = __rdtsc();
    int miss_count = 0;
    while ((__rdtsc() - start) < SAMPLE_WINDOW) {
        uint64_t t1 = __rdtsc();
        (void)*shared_mem;
        _mm_mfence();
        uint64_t latency = __rdtsc() - t1;
        if (latency > THRESHOLD) miss_count++;
    }
    return miss_count > (SAMPLE_WINDOW / 5000);  // 调整阈值
}

int main() {
    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    shared_mem = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    printf("[Receiver] Shared Memory: %p\n", shared_mem);

    while (1) {
        char byte = 0;
        for (int i = 0; i < 8; i++) {
            byte = (byte << 1) | detect_bit();
        }
        printf("Received: %c\n", byte);
        fflush(stdout);
    }

    munmap((void *)shared_mem, 4096);
    return 0;
}