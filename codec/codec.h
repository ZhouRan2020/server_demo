#pragma once
#include <stdint.h>

struct Packet {
    uint16_t cmd;
    uint16_t len;
    const char* data;
};

int check_pack(const char* data, int len);
// 这里不再考虑安全性问题，前面的函数需要保障安全
bool decode(Packet* pack, const char* data, int len);
// 对消息进行编码，返回编码后的长度
int encode(char* buff, uint16_t cmd, const char* data, int len);
