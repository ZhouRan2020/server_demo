#pragma once
#include "../proto/player.pb.h"

using namespace TCCamp;

void player_login();

void player_login_response(const PlayerLoginRsp* rsp);
void player_create();
void player_create_response(const PlayerCreateRsp* rsp);

