#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <x86intrin.h>
#include <dlfcn.h>
#include <unistd.h>
#include <stdbool.h>

#define PREAMBLE 0b101011
#define BIT_INTERVAL 1000000  // 1M cycles
#define FLUSH_COUNT 5         // 每次刷新5个缓存行

void send_bit(volatile char *addr, bool bit, uint64_t duration) {
    uint64_t start = __rdtsc();
    
    while ((__rdtsc() - start) < duration) {
        if (bit) {
            // 增强刷新模式
            for (int i = -FLUSH_COUNT/2; i <= FLUSH_COUNT/2; i++) {
                _mm_clflush((void *)(addr + i*64));
            }
            // 增加计算延迟
            volatile uint64_t x = 1;
            for (int i = 0; i < 500; i++) x += x*x;
        } else {
            // 保持缓存状态
            (void)*addr;
            for (volatile int i = 0; i < 50; i++);
        }
    }
}

void send_message(volatile char *addr, const char *msg) {
    // 发送前导码
    for (int i = 5; i >= 0; i--) {
        send_bit(addr, (PREAMBLE >> i) & 1, BIT_INTERVAL);
    }
    
    // 发送消息主体
    for (int i = 0; msg[i]; i++) {
        for (int j = 7; j >= 0; j--) {
            send_bit(addr, (msg[i] >> j) & 1, BIT_INTERVAL);
        }
    }
    
    // 发送终止符
    for (int i = 0; i < 8; i++) {
        send_bit(addr, 0, BIT_INTERVAL);
    }
}

int main() {
    void *libc_func = dlsym(RTLD_DEFAULT, "printf");
    volatile char *target_addr = (volatile char *)libc_func;
    
    const char *messages[] = {"TEST", "COVERT", "CHANNEL", "EXIT"};
    for (int i = 0; i < 4; i++) {
        printf("[Sender] Transmitting: %s\n", messages[i]);
        send_message(target_addr, messages[i]);
        sleep(1);
    }
    
    return 0;
}