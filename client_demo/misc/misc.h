#pragma once
#include "uv.h"
#include "../codec/codec.h"
#include "../proto/player.pb.h"

struct my_write_t : public uv_write_t {
    uv_buf_t buf;
};

enum CONNECT_STATE {
    CONNECT_STATE_NONE = 0,
    CONNECT_STATE_CONNECTING = 1,
    CONNECT_STATE_CONNECTED = 2,
};

void on_close(uv_handle_t *handle);
void on_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);
void on_write(uv_write_t *req, int status);
void on_connected(uv_connect_t *connect, int status);

void alloc_read_buff(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);
void free_read_buff(const uv_buf_t* buf);
my_write_t* alloc_write_buff(size_t size);
void free_write_buff(my_write_t* w);

void send_to_server(const char* data, int len);
bool send_pb(uint16_t cmd, ::google::protobuf::Message* msg);
void on_netpack_handle(uv_stream_t* stream, const char* buff, int len);
