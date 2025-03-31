#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <x86intrin.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdbool.h>

#define PREAMBLE 0b101011
#define BIT_INTERVAL 1000000
#define FLUSH_COUNT 5

void send_bit(volatile char *addr, bool bit, uint64_t duration) {
    uint64_t start = __rdtsc();
    while ((__rdtsc() - start) < duration) {
        if (bit) {
            _mm_clflush((void *)addr);  // 刷新缓存
            volatile uint64_t x = 1;
            for (int i = 0; i < 1000; i++) x += x * x;  // 增加延迟
        } else {
            (void)*addr;  // 正常访问
            for (volatile int i = 0; i < 100; i++);
        }
    }
}

int main() {
    // 创建共享内存（代替 libc 地址）
    volatile char *shared_mem = mmap(
        NULL, 4096, PROT_READ | PROT_WRITE,
        MAP_SHARED | MAP_ANONYMOUS, -1, 0
    );
    printf("[Sender] Shared Memory Address: %p\n", shared_mem);

    const char *messages[] = {"TEST", "COVERT", "CHANNEL", "EXIT"};
    for (int i = 0; i < 4; i++) {
        printf("[Sender] Transmitting: %s\n", messages[i]);
        
        // 发送前导码 0b101011
        for (int j = 5; j >= 0; j--) {
            bool bit = (PREAMBLE >> j) & 1;
            send_bit(shared_mem, bit, BIT_INTERVAL);
        }
        
        // 发送消息
        for (int k = 0; messages[i][k]; k++) {
            for (int m = 7; m >= 0; m--) {
                bool bit = (messages[i][k] >> m) & 1;
                send_bit(shared_mem, bit, BIT_INTERVAL);
            }
        }
        
        // 发送结束标志（8个0）
        for (int n = 0; n < 8; n++) {
            send_bit(shared_mem, 0, BIT_INTERVAL);
        }
        
        sleep(1);
    }
    
    munmap((void *)shared_mem, 4096);
    return 0;
}