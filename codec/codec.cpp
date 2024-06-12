#pragma once
#include "codec.h"
#include <string.h>
#include <stdlib.h>

#define PROTO_PREFIX "TC"           // 协议前置头，用于快速检验
#define PROTO_PREFIX_LEN strlen(PROTO_PREFIX)       // 前置头的长度
#define PROTO_SIZE_LEN 2            // 协议体长度所占空间
#define PROTO_CMD_LEN 2             // 协议号所占空间    
#define PROTO_HEAD_LEN (PROTO_PREFIX_LEN + PROTO_SIZE_LEN + PROTO_CMD_LEN)      // 协议头部所占空间

int check_pack(const char* data, int len) {
    int msg_len = 0;
    if (len < PROTO_PREFIX_LEN + PROTO_SIZE_LEN) {  // 基本长度不够，表示包不完整，继续等待
        return 0;
    }

    if (memcmp(data, PROTO_PREFIX, PROTO_PREFIX_LEN) != 0) {    // 快速TC检验不通过，返回异常
        return -1;
    }

    msg_len = *((uint16_t*)(data + PROTO_PREFIX_LEN));          // 获取消息体的长度
    if (len >= msg_len + PROTO_PREFIX_LEN + PROTO_SIZE_LEN) {   // 数据包长度够了
        return (int)(msg_len + PROTO_PREFIX_LEN + PROTO_SIZE_LEN);
    }
    else {
        return 0;           // 长度不够，继续等待
    }
}

// 解码，这里不再考虑安全性问题，前面的函数需要保障安全
bool decode(Packet* pack, const char* data, int len) {
    bool result = false;
    if (len < PROTO_HEAD_LEN) {
        goto Exit0;
    }

    pack->cmd = *((uint16_t*)(data + PROTO_PREFIX_LEN + PROTO_SIZE_LEN));
    pack->data = data + PROTO_HEAD_LEN;
    pack->len = (int)(len - PROTO_HEAD_LEN);
    result = true;
Exit0:
    return result;
}

// 对消息进行编码，返回编码后的长度
int encode(char* buff, uint16_t cmd, const char* data, int len) {
    uint16_t bodyLen = len + PROTO_CMD_LEN;
    memcpy(buff, "TC", PROTO_PREFIX_LEN);
    memcpy(buff + PROTO_PREFIX_LEN, &bodyLen, PROTO_SIZE_LEN);
    memcpy(buff + PROTO_PREFIX_LEN + PROTO_SIZE_LEN, &cmd, PROTO_CMD_LEN);
    if (len > 0) {
        memcpy(buff + PROTO_HEAD_LEN, data, len);
    }
    return (int)(len + PROTO_HEAD_LEN);
}
