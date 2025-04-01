#include "util.h"

int CACHE_MISS_LATENCY = 100;  // 可校准或通过参数传递

int main(int argc, char **argv) {
    struct config config;
    init_config(&config, argc, argv);

    // 动态获取 ecvt_r 地址
    void *handle = dlopen("libc.so.6", RTLD_LAZY);
    config.addr = (ADDR_PTR)dlsym(handle, "ecvt_r");
    printf("[Receiver] ecvt_r address: %p\n", (void*)config.addr);

    uint32_t bitSequence = 0;
    uint32_t syncMask = 0xFF;  // 同步序列掩码
    uint32_t expSequence = 0b10101011;  // 期望的同步序列

    printf("Listening...\n");
    while (1) {
        bool bit = detect_bit(&config);
        bitSequence = ((bitSequence << 1) | bit) & syncMask;

        // 检测到同步序列后开始接收消息
        if (bitSequence == expSequence) {
            char binary_msg[MAX_BUFFER_LEN] = {0};
            int strike_zeros = 0;

            for (int i = 0; i < MAX_BUFFER_LEN; i++) {
                binary_msg[i] = detect_bit(&config) ? '1' : '0';
                if (binary_msg[i] == '0' && ++strike_zeros >= 8) break;
            }

            // 二进制转ASCII（复用Git版本的conv_char函数）
            char ascii_msg[MAX_BUFFER_LEN/8 + 1];
            printf("Received: %s\n", conv_char(binary_msg, strlen(binary_msg)/8, ascii_msg));
        }
    }

    dlclose(handle);
    return 0;
}
