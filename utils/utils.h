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
    //itemincfg 需要存储的 是不需要sendpb传输的信息
    //proto的message Item则是存储需要传输的信息
    //name introduce iconName都只需要本地存储而并不需要传输
    // 尤其是introduce很长没必要序列化传输 直接本地查表显示就行了
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