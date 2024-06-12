#include "misc.h"
#include "../codec/codec.h"
#include "logic/logic.h"

using namespace std;

extern int g_connect_state;
extern uv_tcp_t* g_client;
static char s_send_buff[64 * 1024];

void alloc_read_buff(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
    buf->base = (char*)malloc(suggested_size);
    buf->len = (int)suggested_size;
}

void free_read_buff(const uv_buf_t* buf) {
    if (buf->base) {
        free(buf->base);
    }
}

my_write_t* alloc_write_buff(size_t size) {
    my_write_t* w = (my_write_t*)malloc(sizeof(my_write_t));
    if (w == nullptr)
        return nullptr;

    w->buf.base = (char*)malloc(size);
    if (w->buf.base == nullptr) {
        free(w);
        return nullptr;
    }
    w->buf.len = (int)size;

    return w;
}

void free_write_buff(my_write_t* w) {
    if (w->buf.base != nullptr) {
        free(w->buf.base);
    }
    free(w);
}

void on_close(uv_handle_t *handle) {
    if (handle != NULL) {
        free(handle);
    }
    g_client = nullptr;
    g_connect_state = CONNECT_STATE_NONE;

    fprintf(stdout, "on close handle\n");
}

void on_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
    if (nread < 0) {
        if (nread != UV_EOF) {
            fprintf(stderr, "Read error %s\n", uv_err_name((int)nread));
        }
        else {
            fprintf(stderr, "client disconnect\n");
        }
        uv_close((uv_handle_t*)stream, on_close);
    }
    else {
        on_netpack_handle(stream, buf->base, (int)nread);
    }

    free_read_buff(buf);
}

void on_write(uv_write_t *req, int status) {
    if (status) {
        fprintf(stderr, "Write error %s\n", uv_strerror(status));
        uv_close((uv_handle_t*)req->handle, on_close);
        goto Exit0;
    }
Exit0:
    free_write_buff((my_write_t*)req);
}

void on_connected(uv_connect_t *connect, int status) {
    if (status < 0) {
        fprintf(stderr, "connect server failed: %s\n", uv_strerror(status));
        g_connect_state = CONNECT_STATE_NONE;
        goto Exit0;
    }
    uv_read_start(connect->handle, alloc_read_buff, on_read);

    g_connect_state = CONNECT_STATE_CONNECTED;

    fprintf(stdout, "connect server success\n");

    player_login();
Exit0:
    free(connect);
}

void send_to_server(const char* data, int len) {
    int retCode = 0;

    my_write_t* req = nullptr;
    if (g_connect_state != CONNECT_STATE_CONNECTED || g_client == nullptr) {
        goto Exit0;
    }

    req = alloc_write_buff(len);
    if (req == nullptr) {
        goto Exit0;
    }
    memcpy(req->buf.base, data, len);

    //发送buffer数组，第四个参数表示数组大小
    retCode = uv_write(req, (uv_stream_t*)g_client, &(req->buf), 1, on_write);
    if (retCode != 0) {
        goto Exit0;
    }
    req = nullptr;
Exit0:
    if (req) {
        free_write_buff(req);
        req = nullptr;
    }
}

bool send_pb(uint16_t cmd, ::google::protobuf::Message* msg) {
    string data = msg->SerializeAsString();
    int len = encode(s_send_buff, cmd, data.c_str(), (int)data.length());
    send_to_server(s_send_buff, len);
    return true;
}

void on_netpack_handle(uv_stream_t* stream, const char* buff, int len) {
    Packet pack;
    decode(&pack, buff, len);
    switch (pack.cmd) {
        case SERVER_PONG:
            fprintf(stdout, "server pong\n");
            break;
        case SERVER_ADD_RSP:
        {
            AddRsp rsp;
            rsp.ParseFromArray(pack.data, pack.len);
            printf("server rsponse: %d + %d = %d\n", rsp.a(), rsp.b(), rsp.result());
            break;
        }
        case GM_OPERATE_RSP:
            printf("recv gm response:%s\n", pack.data);
            break;
        case SERVER_LOGIN_RSP:
        {
            PlayerLoginRsp rsp;
            rsp.ParseFromArray(pack.data, pack.len);
            player_login_response(&rsp);
            break;
        }
        case SERVER_CREATE_RSP:
        {
            PlayerCreateRsp rsp;
            rsp.ParseFromArray(pack.data, pack.len);
            player_create_response(&rsp);
            break;
        }
        case SERVER_ANNOUNCE_RSP:
        {
            SyncAnnounce sync;
            sync.ParseFromArray(pack.data, pack.len);
            fprintf(stdout, "Announce:\n%s\n", sync.announce().c_str());
            break;
        }
        default:
            break;
    }
}

