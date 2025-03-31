// util.h
#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>      // uint64_t 定义
#include <x86intrin.h>   // __rdtsc 和 _mm_mfence
#include <string.h>      // strlen 和 memcmp 声明

static inline uint64_t measure_access(volatile char *addr) {
    uint64_t start = __rdtsc();
    (void)*addr;
    _mm_mfence();
    return __rdtsc() - start;
}

static inline void binary_to_ascii(const char *binary, char *output) {
    size_t len = strlen(binary);
    for (size_t i = 0; i < len/8; i++) {
        char byte = 0;
        for (int j = 0; j < 8; j++) {
            if (i*8+j < len) {
                byte = (byte << 1) | (binary[i*8+j] == '1');
            }
        }
        output[i] = byte;
    }
    output[len/8] = '\0';
}

#endif