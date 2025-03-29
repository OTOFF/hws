/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * threshold.c
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * Flush + Reload covert channel threshold tuner
 * Loops indefinitely, printing statistics periodically.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
 /* Enhanced Threshold Calibration Tool */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>
#include <x86intrin.h>
#include <math.h>
#include <time.h>
#include <immintrin.h>

typedef struct {
    uint64_t count;
    double sum;
    long double sum_sq;
} timing_stats;

void update_stats(timing_stats *stats, uint64_t t) {
    stats->count++;
    stats->sum += t;
    stats->sum_sq += (long double)t * t;
}

void print_stats(const char *label, timing_stats *stats) {
    if (stats->count == 0) return;
    double avg = stats->sum / stats->count;
    double variance = (stats->sum_sq / stats->count) - (avg * avg);
    double stddev = sqrt(variance);
    printf("%s: count=%lu avg=%.2f cycles stddev=%.2f cycles\n",
           label, stats->count, avg, stddev);
}

// 新增：测量指令时序差异
void test_instruction_timing(timing_stats *fast, timing_stats *slow) {
    volatile uint64_t a = 1;
    for (int i = 0; i < 100; i++) {
        // 快速指令（加法）
        uint64_t start = __rdtsc();
        for (int j = 0; j < 10; j++) a += 1;
        update_stats(fast, __rdtsc() - start);

        // 慢速指令（除法）
        start = __rdtsc();
        for (int j = 0; j < 10; j++) a /= 3;
        update_stats(slow, __rdtsc() - start);
    }
}

int main(void) {
    printf("[Threshold] Calibrating enhanced channel...\n");

    timing_stats cache_flushed = {0}, cache_normal = {0};
    timing_stats inst_fast = {0}, inst_slow = {0};
    timing_stats port_int = {0}, port_avx = {0};

    // 原有缓存测试逻辑
    void *libc_fn = /* ... 获取libc地址 ... */;

    for (int i = 0; i < 100; i++) {
        // 缓存测试
        _mm_clflush(libc_fn);
        uint64_t start = __rdtsc();
        /* ... 原有缓存测试代码 ... */
        update_stats(&cache_flushed, __rdtsc() - start);

        // 指令时序测试
        test_instruction_timing(&inst_fast, &inst_slow);

        // 端口争用测试
        volatile uint64_t a = 1;
        start = __rdtsc();
        for (int j = 0; j < 10; j++) a += 1;
        update_stats(&port_int, __rdtsc() - start);

        __m256i vec = _mm256_set1_epi32(1);
        start = __rdtsc();
        for (int j = 0; j < 10; j++) vec = _mm256_add_epi32(vec, vec);
        update_stats(&port_avx, __rdtsc() - start);
    }

    // 打印结果
    printf("\n=== Cache Timing ===\n");
    print_stats("Flushed", &cache_flushed);
    print_stats("Normal", &cache_normal);

    printf("\n=== Instruction Timing ===\n");
    print_stats("Fast (add)", &inst_fast);
    print_stats("Slow (div)", &inst_slow);

    printf("\n=== Port Contention ===\n");
    print_stats("Integer Port", &port_int);
    print_stats("AVX Port", &port_avx);

    // 计算建议阈值
    int suggested_threshold = (int)((cache_flushed.sum / cache_flushed.count + 
                                   inst_slow.sum / inst_slow.count +
                                   port_avx.sum / port_avx.count) / 3);
    printf("\nSuggested threshold: %d cycles\n", suggested_threshold);

    return 0;
}