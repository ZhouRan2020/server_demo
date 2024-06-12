#pragma once
#include <stdint.h>

// ����������ݵ���ǰ���̵�./dataĿ¼�£���playerIDΪ�ļ���
// @playerID ���ID
// @data Ҫ���������buff
// @len data�ĳ���
// ����ֵ���ɹ�����0��ʧ�ܷ��ظ���
int save(const char* playerID, const char* data, int len);
// ����������ݣ��ɹ��������ݳ��ȣ�ʧ�ܷ��ظ���
// @playerID ���ID
// @data ����buff
// @size buff�Ĵ�С
// return: �ɹ����ؼ������ݵĳ��ȣ�ʧ��Ϊ������
// -1 �ļ�������
// -2 ���岻����
int load(const char* playerID, char* data, int size);

// ��ȡ�����ļ�
// @path �ļ�·����
// @data ����buff�����ָ��null����ֻ�����ļ�����
// @size buff�Ĵ�С
// return: �ɹ����ؼ������ݵĳ��ȣ�ʧ��Ϊ������
// -1 �ļ�������
// -2 ���岻����
int readCfg(const char* path, char* data, int size);
