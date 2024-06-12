#include <stdio.h>
#include "uv.h"
#include "misc/misc.h"
#include "math.h"
#include "logic/logic.h"

//////////////////////////////////////////////////////////////////////////

uv_loop_t* g_loop = nullptr;
int g_connect_state = CONNECT_STATE_NONE;
uv_tcp_t* g_client = nullptr;
static char s_send_buff[64 * 1024];

void try_connect_server() {
    sockaddr_in addr;
    uv_connect_t *connect = nullptr;
    uv_tcp_t* client = nullptr;

    if (g_connect_state == CONNECT_STATE_NONE) {
        client = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
        uv_tcp_init(g_loop, client);

        uv_ip4_addr("127.0.0.1", 8086, &addr);

        connect = (uv_connect_t*)malloc(sizeof(uv_connect_t));
        if (0 != uv_tcp_connect(connect, client, (sockaddr*)&addr, on_connected)) {
            uv_close((uv_handle_t*)client, NULL);
            goto Exit0;
        }
        else {
            g_client = client;
            connect = nullptr;
            client = nullptr;
            g_connect_state = CONNECT_STATE_CONNECTING;
        }
    }
Exit0:
    if (client) {
        free(client);
        client = nullptr;
    }
    if (connect) {
        free(connect);
        connect = nullptr;
    }
}


void on_timer(uv_timer_t* handle) {
    if (g_connect_state != CONNECT_STATE_CONNECTED)
        return;
    /*{
        int len = encode(s_send_buff, CLIENT_PING, nullptr, 0);
        send_to_server(s_send_buff, len);
    }
    {
        int len = encode(s_send_buff, CLIENT_ANNOUNCE_REQ, nullptr, 0);
        send_to_server(s_send_buff, len);
    }*/
}

void main_loop(uv_idle_t* handle) {
    if (g_connect_state == CONNECT_STATE_NONE) {
        try_connect_server();
    }
    else if (g_connect_state == CONNECT_STATE_CONNECTED) {
        
    }

    Sleep(3000);
}

int main(int argc, char* argv[]) {
    time_t now;
    time(&now);
    srand((uint32_t)now);

    g_loop = uv_default_loop();

    uv_idle_t idler;
    uv_idle_init(g_loop, &idler);
    uv_idle_start(&idler, main_loop);

    uv_timer_t timer;
    uv_timer_init(g_loop, &timer);
    uv_timer_start(&timer, on_timer, 6000, 6000);

    uv_run(g_loop, UV_RUN_DEFAULT);

    printf("main loop stop\n");
    return 0;
}


