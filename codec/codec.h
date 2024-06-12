#pragma once
#include <stdint.h>

struct Packet {
    uint16_t cmd;
    uint16_t len;
    const char* data;
};

int check_pack(const char* data, int len);
// ���ﲻ�ٿ��ǰ�ȫ�����⣬ǰ��ĺ�����Ҫ���ϰ�ȫ
bool decode(Packet* pack, const char* data, int len);
// ����Ϣ���б��룬���ر����ĳ���
int encode(char* buff, uint16_t cmd, const char* data, int len);
