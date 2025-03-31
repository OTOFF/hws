#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <x86intrin.h>
#include <dlfcn.h>
#include <unistd.h>
#include <stdbool.h>

// 确保 RTLD_DEFAULT 有定义
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
            for (int i = -FLUSH_COUNT/2; i <= FLUSH_COUNT/2; i++) {
                _mm_clflush((void *)(addr + i*64));
            }
            volatile uint64_t x = 1;
            for (int i = 0; i < 500; i++) x += x*x;
        } else {
            (void)*addr;
            for (volatile int i = 0; i < 50; i++);
        }
    }
}

int main() {
    // 更安全的加载方式
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
    const char *messages[] = {"TEST", "COVERT", "CHANNEL", "EXIT"};
    
    for (int i = 0; i < 4; i++) {
        printf("[Sender] Transmitting: %s\n", messages[i]);
        
        // 发送前导码
        for (int j = 5; j >= 0; j--) {
            send_bit(target_addr, (PREAMBLE >> j) & 1, BIT_INTERVAL);
        }
        
        // 发送消息
        for (int k = 0; messages[i][k]; k++) {
            for (int m = 7; m >= 0; m--) {
                send_bit(target_addr, (messages[i][k] >> m) & 1, BIT_INTERVAL);
            }
        }
        
        // 发送结束标志
        for (int n = 0; n < 8; n++) {
            send_bit(target_addr, 0, BIT_INTERVAL);
        }
        
        sleep(1);
    }
    
    dlclose(handle);
    return 0;
}