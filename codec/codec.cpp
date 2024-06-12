#pragma once
#include "codec.h"
#include <string.h>
#include <stdlib.h>

#define PROTO_PREFIX "TC"           // Э��ǰ��ͷ�����ڿ��ټ���
#define PROTO_PREFIX_LEN strlen(PROTO_PREFIX)       // ǰ��ͷ�ĳ���
#define PROTO_SIZE_LEN 2            // Э���峤����ռ�ռ�
#define PROTO_CMD_LEN 2             // Э�����ռ�ռ�    
#define PROTO_HEAD_LEN (PROTO_PREFIX_LEN + PROTO_SIZE_LEN + PROTO_CMD_LEN)      // Э��ͷ����ռ�ռ�

int check_pack(const char* data, int len) {
    int msg_len = 0;
    if (len < PROTO_PREFIX_LEN + PROTO_SIZE_LEN) {  // �������Ȳ�������ʾ���������������ȴ�
        return 0;
    }

    if (memcmp(data, PROTO_PREFIX, PROTO_PREFIX_LEN) != 0) {    // ����TC���鲻ͨ���������쳣
        return -1;
    }

    msg_len = *((uint16_t*)(data + PROTO_PREFIX_LEN));          // ��ȡ��Ϣ��ĳ���
    if (len >= msg_len + PROTO_PREFIX_LEN + PROTO_SIZE_LEN) {   // ���ݰ����ȹ���
        return (int)(msg_len + PROTO_PREFIX_LEN + PROTO_SIZE_LEN);
    }
    else {
        return 0;           // ���Ȳ����������ȴ�
    }
}

// ���룬���ﲻ�ٿ��ǰ�ȫ�����⣬ǰ��ĺ�����Ҫ���ϰ�ȫ
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

// ����Ϣ���б��룬���ر����ĳ���
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
