#include "util.h"

bool sequence[8] = {1,0,1,0,1,0,1,1};  // 同步序列

int main(int argc, char **argv) {
    // 初始化配置（包括目标地址和间隔）
    struct config config;
    init_config(&config, argc, argv);

    // 动态获取 ecvt_r 地址（替换硬编码）
    void *handle = dlopen("libc.so.6", RTLD_LAZY);
    config.addr = (ADDR_PTR)dlsym(handle, "ecvt_r");
    printf("[Sender] ecvt_r address: %p\n", (void*)config.addr);

    const char *msg = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    char *binary_msg = string_to_binary(msg);  // 复用Git版本的字符串转二进制函数

    // 发送同步序列 + 消息
    for (int i = 0; i < 8; i++) {
        send_bit(sequence[i], &config);  // 发送同步头 10101011
    }

    // 发送消息比特流
    for (int i = 0; binary_msg[i] != '\0'; i++) {
        send_bit(binary_msg[i] == '1', &config);
    }

    dlclose(handle);
    return 0;
}
