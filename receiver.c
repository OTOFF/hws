#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <x86intrin.h>
#include <math.h>
#include <time.h>
#include <dlfcn.h>
#include <stdbool.h>    // 添加bool类型定义

#define CACHE_MISS_THRESHOLD 1847
#define PREAMBLE 0b101011
#define MAX_MSG_LEN 1024

static inline uint64_t cc_sync() {
    _mm_mfence();
    return __rdtsc();
}

static inline uint64_t measure_access(volatile char *addr) {
    uint64_t start = __rdtsc();
    (void)*addr;
    _mm_mfence();
    return __rdtsc() - start;
}

bool detect_bit(volatile char *addr, uint64_t interval) {
    int misses = 0;
    uint64_t start = cc_sync();
    
    while ((__rdtsc() - start) < interval) {
        uint64_t latency = measure_access(addr);
        if (latency > CACHE_MISS_THRESHOLD) misses++;
    }
    return misses >= 5;
}

void binary_to_ascii(const char *binary, char *output) {
    for (int i = 0; i < strlen(binary)/8; i++) {
        char byte = 0;
        for (int j = 0; j < 8; j++) {
            byte = (byte << 1) | (binary[i*8+j] == '1');
        }
        output[i] = byte;
    }
    output[strlen(binary)/8] = '\0';
}

int main() {
    void *libc_func = dlsym(RTLD_DEFAULT, "printf");
    volatile char *target_addr = (volatile char *)libc_func;
    
    uint32_t bit_sequence = 0;
    char binary_msg[MAX_MSG_LEN] = {0};
    int msg_index = 0;
    int zero_streak = 0;
    
    printf("[Receiver] Waiting for preamble (101011)...\n");
    
    while (1) {
        bool bit = detect_bit(target_addr, 100000);
        bit_sequence = ((bit_sequence << 1) | bit) & 0x3F;
        
        if ((bit_sequence ^ PREAMBLE) == 0) {
            printf("Preamble detected! Starting message reception...\n");
            
            while (msg_index < MAX_MSG_LEN - 8) {
                bit = detect_bit(target_addr, 100000);
                binary_msg[msg_index++] = bit ? '1' : '0';
                
                if (bit) {
                    zero_streak = 0;
                } else if (++zero_streak >= 8) {
                    break;
                }
            }
            
            char ascii_msg[MAX_MSG_LEN/8 + 1];
            binary_to_ascii(binary_msg, ascii_msg);
            printf("Received: %s\n", ascii_msg);
            
            if (strstr(ascii_msg, "exit")) break;
            memset(binary_msg, 0, sizeof(binary_msg));
            msg_index = 0;
            zero_streak = 0;
        }
    }
    
    printf("Receiver shutdown.\n");
    return 0;
}