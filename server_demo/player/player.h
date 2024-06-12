#pragma once
#include <stdint.h>
#include <string>
#include <map>
#include <vector>
#include "uv.h"
#include "../proto/player.pb.h"
#include "../proto/syncMessage.pb.h"

using namespace std;
using namespace TCCamp;


extern struct Player {
    /*
message PlayerSyncInfo
    Float3 Position = 1;
    Float4 Rotation = 2;
    float Speed = 3;
    int32 UID = 4;
    */
    string PlayerID;
    string Password;
    string Name;
    Float3 Position;
    Float4 Rotation;
    float Speed;

    vector<Item> playerItems;
    int money;
};

class PlayerMgr {
public:
    PlayerMgr();
    ~PlayerMgr();

    bool init();
    bool un_init();

    // 处理用户请求
    bool player_login(uv_tcp_t* client, const PlayerLoginReq* req);
    bool player_create(uv_tcp_t* client, const PlayerCreateReq* req);
    bool send_shop_content(uv_tcp_t* client, const GetShopContentReq* req);
    bool send_backpack_content(uv_tcp_t* client, const GetBackPackContentReq* req);
    bool buy_item(uv_tcp_t* client, const BuyItemReq* req);
    bool announce_request(uv_tcp_t* client);

    Player* find_player(string playerID);

    // 有链接断开
    bool on_client_close(uv_tcp_t* client);
    
    // 广播公告
    bool broadcast_announce(string announce);
private:
    bool _load_player_backpack(string playerID, Player* playerData);
    bool _load_player(string playerID, PlayerSaveData* playerData);
    bool _save_player(const Player* player);
public:
    map<uv_tcp_t*, Player*> m_playerMap;
    string m_announce;
};
void generate_new_login_player_by_saveData(Player* toWrite, PlayerSaveData& data);