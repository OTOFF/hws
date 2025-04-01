#include "util.h"
#include <string.h>
#include <stdio.h>
#include <dlfcn.h>

// ----------- 核心函数 -----------
void clflush(ADDR_PTR addr) {
    _mm_clflush((void *)addr);
    _mm_mfence();
}

CYCLES rdtscp() {
    unsigned aux;
    return __rdtscp(&aux);
}

CYCLES measure_one_block_access_time(ADDR_PTR addr) {
    volatile char *ptr = (volatile char *)addr;
    CYCLES start = rdtscp();
    (void)*ptr;  // 强制内存访问
    return rdtscp() - start;
}

CYCLES cc_sync() {
    CYCLES t0, t1;
    do {
        t0 = rdtscp();
        t1 = rdtscp();
    } while ((t1 >> 12) == (t0 >> 12));  // 按 2^12 周期对齐
    return t1;
}

// ----------- 通信逻辑 -----------
bool detect_bit(ADDR_PTR addr, int interval) {
    int misses = 0, hits = 0;
    CYCLES start = cc_sync();
    while (rdtscp() - start < interval) {
        CYCLES t = measure_one_block_access_time(addr);
        (t > 100) ? misses++ : hits++;  // 阈值固定为100（可调整）
    }
    return misses > hits;
}

void send_bit(bool bit, ADDR_PTR addr, int interval) {
    CYCLES start = cc_sync();
    if (bit) {
        while (rdtscp() - start < interval) {
            clflush(addr);
        }
    } else {
        while (rdtscp() - start < interval) {}  // 空循环表示0
    }
}

// ----------- 二进制转换 -----------
char *string_to_binary(const char *str) {
    static char binary[1024];
    binary[0] = '\0';
    for (size_t i = 0; str[i] != '\0'; i++) {
        for (int j = 7; j >= 0; j--) {
            strcat(binary, (str[i] & (1 << j)) ? "1" : "0");
        }
    }
    return binary;
}

char binary_to_char(const char *binary) {
    char c = 0;
    for (int i = 0; i < 8; i++) {
        if (binary[i] == '1') c |= (1 << (7 - i));
    }
    return c;
}
