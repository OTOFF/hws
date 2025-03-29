/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * sender.c
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * Flush + Reload covert channel sender (instruction page)
 * Loops indefinitely, sending bits until killed.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
 /* Flush + Reload + Instruction Timing/Port Contention Sender */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>
#include <x86intrin.h>
#include <string.h>
#include <immintrin.h>  // 新增：AVX指令支持

int main(void) {
    printf("[Sender] Starting with enhanced timing/port contention...\n");
    fflush(stdout);

    // 加载libc函数地址（原有逻辑）
    void *handle = dlopen("libc.so.6", RTLD_LAZY);
    if (!handle) {
        fprintf(stderr, "[Sender] dlopen failed\n");
        return 1;
    }
    void *libc_fn = dlsym(handle, "ecvt_r");
    if (!libc_fn) {
        fprintf(stderr, "[Sender] dlsym failed\n");
        return 1;
    }
    printf("[Sender] libc address = %p\n", libc_fn);
    fflush(stdout);
    dlclose(handle);

    const char *msg = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    size_t msg_len = strlen(msg);
    size_t num_bits = msg_len << 3;
    unsigned long bit_index = 0;

    // 新增：定义用于端口争用的AVX向量
    __m256i avx_vec = _mm256_set1_epi32(1);

    while (1) {
        size_t char_index = bit_index >> 3;
        int bit_pos = bit_index & 7;
        int bit_to_send = (msg[char_index] >> bit_pos) & 1;

        // 方法1：原有缓存刷新逻辑
        _mm_clflush((char *)libc_fn);
        _mm_clflush((char *)libc_fn + 64);

        // 方法2：新增指令时序差异
        volatile uint64_t timing_var = 1;
        if (bit_to_send) {
            for (int i = 0; i < 10; i++) timing_var /= 3;  // 高延迟除法
        } else {
            for (int i = 0; i < 10; i++) timing_var += 1;  // 低延迟加法
        }

        // 方法3：新增端口争用
        if (bit_to_send) {
            // 占用整数运算端口
            volatile uint64_t port_var = 1;
            for (int i = 0; i < 10; i++) port_var += 1;
        } else {
            // 占用向量端口
            avx_vec = _mm256_add_epi32(avx_vec, avx_vec);
        }

        bit_index = (bit_index + 1) % num_bits;
        usleep(1000);  // 控制发送速率
    }
    return 0;
}