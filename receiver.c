/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * receiver.c
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * Flush + Reload covert channel receiver (instruction page)
 * Loops indefinitely, decoding bits until killed.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
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
#include <immintrin.h>  // 新增：AVX指令支持

// 动态阈值（可通过命令行参数调整）
#define DEFAULT_THRESHOLD 150
static int THRESHOLD = DEFAULT_THRESHOLD;

// 新增：测量端口争用的函数
static uint64_t measure_port_contention() {
    volatile uint64_t a = 1;
    uint64_t start = __rdtsc();
    for (int i = 0; i < 10; i++) a += 1;  // 测量整数端口延迟
    return __rdtsc() - start;
}

int main(int argc, char *argv[]) {
    // 解析阈值参数（原有逻辑）
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

    // 新增：用于检测端口争用的变量
    __m256i avx_vec = _mm256_set1_epi32(1);

    while (1) {
        // 方法1：原有缓存检测逻辑
        start = rdtscp64();
        int s = ecvt_r(pi, 20, &decpt, &sign, buf, sizeof(buf));
        end = rdtscp64();
        uint64_t cache_time = end - start;

        // 方法2：新增指令时序检测
        volatile uint64_t timing_var = 1;
        start = __rdtsc();
        for (int i = 0; i < 10; i++) timing_var += 1;
        end = __rdtsc();
        uint64_t timing_delay = end - start;

        // 方法3：新增端口争用检测
        uint64_t port_delay = measure_port_contention();

        // 综合判断（加权决策）
        int cache_bit = (cache_time > THRESHOLD) ? 1 : 0;
        int timing_bit = (timing_delay > THRESHOLD * 1.2) ? 1 : 0;
        int port_bit = (port_delay > THRESHOLD * 1.5) ? 1 : 0;

        // 多数表决（提高可靠性）
        int final_bit = (cache_bit + timing_bit + port_bit >= 2) ? 1 : 0;
        printf("%d", final_bit);
        fflush(stdout);
    }
    return 0;
}