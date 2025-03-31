#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <x86intrin.h>
#include <dlfcn.h>
#include <stdbool.h>
#include "util.h"

#ifndef RTLD_DEFAULT
#define RTLD_DEFAULT ((void *)0)
#endif

#define PREAMBLE 0b101011
#define MAX_MSG_LEN 1024
#define SAMPLE_WINDOW 1000000
#define DEBUG_MODE 1

typedef struct {
    int cache_threshold;
    int inst_threshold;
    int port_threshold;
} Thresholds;

static inline uint64_t cc_sync() {
    _mm_mfence();
    return __rdtsc();
}

void calibrate(volatile char *addr, Thresholds *t) {
    uint64_t cache_flush = 0, cache_normal = 0;
    uint64_t inst_fast = 0, inst_slow = 0;
    
    for (int i = 0; i < 100; i++) {
        _mm_clflush((void *)addr);
        cache_flush += measure_access(addr);
        cache_normal += measure_access(addr);
    }

    volatile uint64_t x = 1;
    for (int i = 0; i < 100; i++) {
        uint64_t start = __rdtsc();
        x += 1;
        inst_fast += __rdtsc() - start;
        
        start = __rdtsc();
        x /= 3;
        inst_slow += __rdtsc() - start;
    }
    
    t->cache_threshold = (cache_flush/100 + cache_normal/100) / 2;
    t->inst_threshold = (inst_fast/100 + inst_slow/100) / 2;
    t->port_threshold = t->inst_threshold + 10;
    
    if (DEBUG_MODE) {
        printf("Calibrated Thresholds:\n");
        printf("  Cache: %d\n  Inst: %d\n  Port: %d\n",
              t->cache_threshold, t->inst_threshold, t->port_threshold);
    }
}

bool detect_bit(volatile char *addr, Thresholds *t) {
    int votes = 0;
    uint64_t start = cc_sync();
    
    while ((__rdtsc() - start) < SAMPLE_WINDOW) {
        uint64_t lat = measure_access(addr);
        if (lat > t->cache_threshold) votes++;
    }
    return votes > (SAMPLE_WINDOW / 20000);
}

int main() {
    void *handle = dlopen("libc.so.6", RTLD_LAZY);
    if (!handle) {
        fprintf(stderr, "Error loading libc: %s\n", dlerror());
        return 1;
    }
    
    void *libc_func = dlsym(handle, "printf");
    if (!libc_func) {
        fprintf(stderr, "Error finding printf: %s\n", dlerror());
        dlclose(handle);
        return 1;
    }

    volatile char *target_addr = (volatile char *)libc_func;
    Thresholds t;
    calibrate(target_addr, &t);
    
    printf("[Receiver] Ready (Addr: %p)\n", target_addr);
    
    uint32_t bit_seq = 0;
    char binary_msg[MAX_MSG_LEN] = {0};
    int msg_idx = 0;
    
    while (1) {
        bool bit = detect_bit(target_addr, &t);
        bit_seq = ((bit_seq << 1) | bit) & 0x3F;
        
        if ((bit_seq ^ PREAMBLE) == 0) {
            printf("\nPreamble detected!\n");
            
            while (msg_idx < MAX_MSG_LEN - 8) {
                bit = detect_bit(target_addr, &t);
                binary_msg[msg_idx++] = bit ? '1' : '0';
                
                if (msg_idx >= 8 && 
                    memcmp(binary_msg + msg_idx - 8, "00000000", 8) == 0) {
                    binary_msg[msg_idx - 8] = '\0';
                    break;
                }
            }
            
            char ascii_msg[MAX_MSG_LEN/8 + 1];
            binary_to_ascii(binary_msg, ascii_msg);
            printf("Received: %s\n", ascii_msg);
            
            if (strstr(ascii_msg, "EXIT")) break;
            msg_idx = 0;
        }
        
        if (DEBUG_MODE) printf(bit ? "1" : "0");
        fflush(stdout);
    }
    
    dlclose(handle);
    printf("\n[Receiver] Shutdown\n");
    return 0;
}