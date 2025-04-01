#ifndef UTIL_H_
#define UTIL_H_

#include <stdint.h>
#include <stdbool.h>
#include <x86intrin.h>  // 用于 _mm_clflush 和 __rdtscp

// 类型定义
typedef uint64_t ADDR_PTR;  // 内存地址类型
typedef uint64_t CYCLES;    // 时间周期类型

// 缓存刷新和时序测量
void clflush(ADDR_PTR addr);                          // 刷新缓存行
CYCLES measure_one_block_access_time(ADDR_PTR addr);  // 测量内存访问时间
CYCLES rdtscp();                                      // 读取时间戳计数器
CYCLES cc_sync();                                     // 同步时钟周期

// 比特检测和通信配置
bool detect_bit(ADDR_PTR addr, int interval);         // 检测单个比特
void send_bit(bool bit, ADDR_PTR addr, int interval); // 发送单个比特

// 二进制转换工具
char *string_to_binary(const char *str);              // 字符串转二进制（ASCII）
char binary_to_char(const char *binary);              // 8位二进制转字符

#endif
