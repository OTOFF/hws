#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <x86intrin.h>
#include <dlfcn.h>
#include <unistd.h>
#include <stdbool.h>

#ifndef RTLD_DEFAULT
#define RTLD_DEFAULT ((void *)0)
#endif

#define PREAMBLE 0b101011
#define BIT_INTERVAL 1000000
#define FLUSH_COUNT 5

void send_bit(volatile char *addr, bool bit, uint64_t duration) {
    uint64_t start = __rdtsc();
    
    while ((__rdtsc() - start) < duration) {
        if (bit) {
            // 更激进的缓存刷新
            for (int i = 0; i < FLUSH_COUNT; i++) {
                _mm_clflush((void *)(addr));
            }
            // 增加计算延迟
            volatile uint64_t x = 1;
            for (int i = 0; i < 1000; i++) x += x * x;
        } else {
            // 0 比特：正常访问
            (void)*addr;
            for (volatile int i = 0; i < 100; i++);
        }
    }
}

int main() {
    void *handle = dlopen("libc.so.6", RTLD_LAZY);
    if (!handle) {
        fprintf(stderr, "Failed to load libc: %s\n", dlerror());
        return 1;
    }
    
    void *libc_func = dlsym(handle, "printf");
    if (!libc_func) {
        fprintf(stderr, "Failed to find printf: %s\n", dlerror());
        dlclose(handle);
        return 1;
    }

    volatile char *target_addr = (volatile char *)libc_func;
    printf("[Sender] Target Address: %p\n", target_addr);  // 打印发送地址

    const char *messages[] = {"TEST", "COVERT", "CHANNEL", "EXIT"};
    
    for (int i = 0; i < 4; i++) {
        printf("[Sender] Transmitting: %s\n", messages[i]);
        
        // 发送前导码 0b101011
        for (int j = 5; j >= 0; j--) {
            bool bit = (PREAMBLE >> j) & 1;
            printf("Sending preamble bit %d\n", bit);  // 打印每个前导比特
            send_bit(target_addr, bit, BIT_INTERVAL);
        }
        
        // 发送消息
        for (int k = 0; messages[i][k]; k++) {
            for (int m = 7; m >= 0; m--) {
                bool bit = (messages[i][k] >> m) & 1;
                send_bit(target_addr, bit, BIT_INTERVAL);
            }
        }
        
        // 发送结束标志（8个0）
        for (int n = 0; n < 8; n++) {
            send_bit(target_addr, 0, BIT_INTERVAL);
        }
        
        sleep(1);
    }
    
    dlclose(handle);
    return 0;
}