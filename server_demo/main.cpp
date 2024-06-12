 #include <stdio.h>
#include "uv.h"
#include "../utils/utils.h"
#include "../netpack/nethandle.h"
#include <map>
#include "player/player.h"
# pragma region ShouldDelete
#include "../cJSON/cJSON.h"
#include "./filedb/filedb.h"
#include <iostream>
#include "room.h"
#pragma endregion

#ifdef WIN32

#else
#include <unistd.h>
#endif

//////////////////////////////////////////////////////////////////////////
using namespace std;

uv_loop_t* g_loop = nullptr;

ServerCfg g_config;
ItemCfg g_item_config;
extern PlayerMgr g_playerMgr;
vector<Room> g_rooms(2);
void update_online_players_syncInfo();

void my_sleep(int t) {
#ifdef WIN32
    Sleep(t);
#else
    sleep(t);
#endif
}

bool init() {
    if (loadConfig() != 0) {
        fprintf(stderr, "load config failed\n");
        return false;
    }
    if (loadItemConfig() != 0) {
        fprintf(stderr, "load item config failded\n");
        return false;
    }
    return true;
}


void main_loop(uv_idle_t* handle) {
    // todo game logic update
    update_online_players_syncInfo();
    my_sleep(2000);
}
 
int main(int argc, char* argv[]) {
    g_loop = uv_default_loop();

    if (!init()) {
        return 1;
    }

    if (init_socket_server(g_config.game_server_ip, g_config.game_server_port, g_loop) != 0) {
        fprintf(stderr, "init game server failed\n");
        return 2;
    }
    fprintf(stdout, "start game server, ip:%s, port:%d\n", g_config.game_server_ip, g_config.game_server_port);

    uv_idle_t idler;
    uv_idle_init(g_loop, &idler);
    uv_idle_start(&idler, main_loop);
    g_playerMgr.init();

    

    uv_run(g_loop, UV_RUN_DEFAULT);

    printf("main loop stop\n");
    return 0;

}

