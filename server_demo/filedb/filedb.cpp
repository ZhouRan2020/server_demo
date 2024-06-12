#pragma once
#include "filedb.h"
#include <stdio.h>
#include <stdlib.h>
#include <string>
#define SAVE_PATH "./data" //D:\realRepo\Server\Server_day3\server_demo\data

int save(const char* playerID, const char* data, int len) {
    int result = -1;
    FILE * fp = nullptr;
    char path[512] = {0};

    sprintf(path, "%s/%s", SAVE_PATH, playerID);
    fp = fopen(path, "wb");
    if (fp == nullptr) {
        result = -1;
        goto Exit0;
    }
    result = (int)fwrite(data, 1, len, fp);
    if (result != len) {
        result = -2;
        goto Exit0;
    }
    result = 0;
Exit0:
    if (fp != nullptr) {
        fclose(fp);
        fp = nullptr;
    }
    return result;
}

int load(const char* playerID, char* data, int size) {
    int result = -1;
    FILE * fp = nullptr;
    char path[512] = {0};
    int fileSize = 0;

    sprintf(path, "%s/%s", SAVE_PATH, playerID);//这样看，每个playerID将会变成一个JSON文件
    fp = fopen(path, "rb");
    if (fp == nullptr) {
        result = -1; //文件不存在
        goto Exit0;
    }
    fseek(fp, 0, SEEK_END);
    fileSize = ftell(fp);
    if (size < fileSize) {
        result = -2; //给定参数size不足以读完file
        goto Exit0;
    }
    rewind(fp);

    result = (int)fread(data, 1, fileSize, fp);
    if (result != fileSize) {
        result = -3; //读取出错
        goto Exit0;
    }

Exit0:
    if (fp != nullptr) {
        fclose(fp);
        fp = nullptr;
    }
    return result;
}

// 读取配置文件
int readCfg(const char* path, char* data, int size) {
    int result = -1;
    FILE * fp = nullptr;
    int fileSize = 0;

    fp = fopen(path, "rb");
    if (fp == nullptr) {
        result = -1;
        goto Exit0;
    }
    fseek(fp, 0, SEEK_END);
    fileSize = ftell(fp);
    if (data == nullptr) {
        result = fileSize;
        goto Exit0;
    }

    if (size < fileSize) {
        result = -2;
        goto Exit0;
    }
    rewind(fp);

    result = (int)fread(data, 1, fileSize, fp);
    if (result != fileSize) {
        result = -3;
        goto Exit0;
    }

Exit0:
    if (fp != nullptr) {
        fclose(fp);
        fp = nullptr;
    }
    return result;
}
