#pragma once
#include "uv.h"
#include <string>
#include <map>
#include "player/player.h"
void alloc_read_buff(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);
void free_read_buff(const uv_buf_t* buf);

struct my_write_t : public uv_write_t {
    uv_buf_t buf;
};

struct PeerData {
    char recv_buf[64 * 1024];
    int nowPos;
};

struct ServerCfg {
    char game_server_ip[32];
    int32_t game_server_port;
    std::string default_announce;
};
struct ItemInCfg {
    std::string name;
    std::string introduce;
    int32_t price;
    std::string iconName; 
    //itemincfg ��Ҫ�洢�� �ǲ���Ҫsendpb�������Ϣ
    //proto��message Item���Ǵ洢��Ҫ�������Ϣ
    //name introduce iconName��ֻ��Ҫ���ش洢��������Ҫ����
    // ������introduce�ܳ�û��Ҫ���л����� ֱ�ӱ��ز����ʾ������
    /*
    message Item {
    int32 ItemID = 1;
    int32 Price = 2;
	int32 HeapSize = 3;
}
    */
};
struct ItemCfg {
    std::map<int32_t, ItemInCfg> ItemDictionaryByItemId;
};


my_write_t* alloc_write_buff(size_t size);
void free_write_buff(my_write_t* w);

PeerData* malloc_peer_data();
void free_peer_data(PeerData* peerData);

void on_new_connection(uv_stream_t *server, int status);
void on_close(uv_handle_t *handle);
void on_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);
void on_write(uv_write_t *req, int status);

bool sendData(uv_stream_t* stream, const char* data, int len);

int loadConfig();
int loadItemConfig();
int init_socket_server(const char* ip, uint16_t port, uv_loop_t* loop);
std::string TransformStructToJsonStr(Player p);
std::string TransformStructToJsonStr(Item item);
Player TransformJsonStrToStruct(string jsonstr);
Item TransformJsonStrToStruct_Item(string jsonstr);