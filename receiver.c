/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * receiver.c
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * Flush + Reload covert channel receiver (instruction page)
 * Loops indefinitely, decoding bits until killed.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* Enhanced Receiver with Timing/Port Contention Detection */
/* Enhanced Receiver with Timing/Port Contention Detection */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <x86intrin.h>
#include <math.h>
#include <time.h>
#include <immintrin.h>

// 修正：添加 rdtscp64 函数定义
static inline uint64_t rdtscp64() {
    unsigned aux;
    return __rdtscp(&aux);
}

#define DEFAULT_THRESHOLD 150
static int THRESHOLD = DEFAULT_THRESHOLD;

static uint64_t measure_port_contention() {
    volatile uint64_t a = 1;
    uint64_t start = __rdtsc();
    for (int i = 0; i < 10; i++) a += 1;
    return __rdtsc() - start;
}

int main(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-t") == 0 && (i + 1) < argc) {
            THRESHOLD = atoi(argv[++i]);
        }
    }
    printf("[Receiver] Starting with threshold=%d\n", THRESHOLD);
    fflush(stdout);

    double pi = 3.141592653589793;
    int decpt = 0, sign = 0;
    char buf[64];
    uint64_t start, end;
    __m256i avx_vec = _mm256_set1_epi32(1);

    while (1) {
        // 方法1：缓存检测
        start = rdtscp64();
        int s = ecvt_r(pi, 20, &decpt, &sign, buf, sizeof(buf));
        end = rdtscp64();
        uint64_t cache_time = end - start;

        // 方法2：指令时序检测
        volatile uint64_t timing_var = 1;
        start = __rdtsc();
        for (int i = 0; i < 10; i++) timing_var += 1;
        end = __rdtsc();
        uint64_t timing_delay = end - start;

        // 方法3：端口争用检测
        uint64_t port_delay = measure_port_contention();

        int cache_bit = (cache_time > THRESHOLD) ? 1 : 0;
        int timing_bit = (timing_delay > THRESHOLD * 1.2) ? 1 : 0;
        int port_bit = (port_delay > THRESHOLD * 1.5) ? 1 : 0;

        int final_bit = (cache_bit + timing_bit + port_bit >= 2) ? 1 : 0;
        printf("%d", final_bit);
        fflush(stdout);
    }
    return 0;
}