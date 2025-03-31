#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <x86intrin.h>
#include <dlfcn.h>      // 包含RTLD_DEFAULT定义
#include <unistd.h>     // sleep()
#include <stdbool.h>    // bool类型

// 如果RTLD_DEFAULT仍未定义，手动定义
#ifndef RTLD_DEFAULT
#define RTLD_DEFAULT ((void *)0)
#endif

#define PREAMBLE 0b101011
#define BIT_INTERVAL 100000

void send_bit(volatile char *addr, bool bit, uint64_t duration) {
    uint64_t start = __rdtsc();
    while ((__rdtsc() - start) < duration) {
        if (bit) {
            _mm_clflush((void *)addr);
            for (volatile int i = 0; i < 100; i++);
        } else {
            (void)*addr;
            for (volatile int i = 0; i < 10; i++);
        }
    }
}

void send_message(volatile char *addr, const char *msg) {
    // 发送前导码101011
    for (int i = 5; i >= 0; i--) {
        send_bit(addr, (PREAMBLE >> i) & 1, BIT_INTERVAL);
    }
    
    // 发送消息内容
    for (int i = 0; msg[i]; i++) {
        for (int j = 7; j >= 0; j--) {
            send_bit(addr, (msg[i] >> j) & 1, BIT_INTERVAL);
        }
    }
    
    // 发送结束标志（8个0）
    for (int i = 0; i < 8; i++) {
        send_bit(addr, 0, BIT_INTERVAL);
    }
}

int main() {
    void *libc_func = dlsym(RTLD_DEFAULT, "printf");
    if (!libc_func) {
        fprintf(stderr, "Failed to find printf function\n");
        return 1;
    }
    
    volatile char *target_addr = (volatile char *)libc_func;
    const char *messages[] = {"Hello", "Covert", "Channel", "exit"};
    
    for (int i = 0; i < sizeof(messages)/sizeof(messages[0]); i++) {
        printf("Sending: %s\n", messages[i]);
        send_message(target_addr, messages[i]);
        sleep(1);
    }
    
    return 0;
}