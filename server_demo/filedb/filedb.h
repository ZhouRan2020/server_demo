#pragma once
#include <stdint.h>

// 保存玩家数据倒当前进程的./data目录下，以playerID为文件名
// @playerID 玩家ID
// @data 要保存的数据buff
// @len data的长度
// 返回值：成功返回0，失败返回负数
int save(const char* playerID, const char* data, int len);
// 加载玩家数据，成功返回数据长度，失败返回负数
// @playerID 玩家ID
// @data 加载buff
// @size buff的大小
// return: 成功返回加载数据的长度，失败为负数：
// -1 文件不存在
// -2 缓冲不够大
int load(const char* playerID, char* data, int size);

// 读取配置文件
// @path 文件路径名
// @data 加载buff，如果指定null，则只返回文件长度
// @size buff的大小
// return: 成功返回加载数据的长度，失败为负数：
// -1 文件不存在
// -2 缓冲不够大
int readCfg(const char* path, char* data, int size);
