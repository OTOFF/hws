#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <x86intrin.h>
#include <dlfcn.h>
#include <unistd.h>      // 添加sleep声明
#include <stdbool.h>     // 添加bool类型定义

#define PREAMBLE 0b101011
#define BIT_INTERVAL 100000  // cycles

void send_bit(volatile char *addr, bool bit, uint64_t duration) {
    uint64_t start = __rdtsc();
    while ((__rdtsc() - start) < duration) {
        if (bit) {
            // 制造缓存未命中
            _mm_clflush((void *)addr);
            for (volatile int i = 0; i < 100; i++); // 增加延迟
        } else {
            // 制造缓存命中
            (void)*addr;
            for (volatile int i = 0; i < 10; i++);
        }
    }
}

void send_message(volatile char *addr, const char *msg) {
    // 发送前导码
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
    volatile char *target_addr = (volatile char *)libc_func;
    
    const char *messages[] = {"Hello", "Covert", "Channel", "exit"};
    
    for (int i = 0; i < sizeof(messages)/sizeof(messages[0]); i++) {
        printf("Sending: %s\n", messages[i]);
        send_message(target_addr, messages[i]);
        sleep(1);
    }
    
    return 0;
}