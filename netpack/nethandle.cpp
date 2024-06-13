#include "nethandle.h"
#include "../codec/codec.h"
#include "player/player.h"
#include <map>
#include "../proto/texus_room.pb.h"
#include "room.h"
using namespace std;

static map<uv_tcp_t*, PeerData*> s_clientMap;
static char s_send_buff[64 * 1024];

extern PlayerMgr g_playerMgr;

extern vector<Room> g_rooms;
extern map<string, int> g_id;

// 发送一个pb对象到客户端
bool SendPBToClient(uv_tcp_t* client, uint16_t cmd, ::google::protobuf::Message* msg)
{
	//string data = msg->SerializeAsString();
	//int len = encode(s_send_buff, cmd, data.c_str(), (int)data.length());
	//return sendData((uv_stream_t*)client, s_send_buff, len);
	string data;
	msg->SerializeToString(&data);
	//  cout << data << '\n';
	int len = encode(s_send_buff, cmd, data.c_str(), (int)data.length());
	cout << hex << s_send_buff << '\n';
	return sendData((uv_stream_t*)client, s_send_buff, len);
}

bool OnNewClient(uv_tcp_t* client)
{
	bool result = false;
	PeerData* peerData = malloc_peer_data();
	if (!peerData) {
		goto Exit0;
	}
	s_clientMap.insert(pair<uv_tcp_t*, PeerData*>(client, peerData));
	result = true;
Exit0:
	return result;
}

bool OnCloseClient(uv_tcp_t* client)
{
	cout << "OnCloseClient" << endl;
	g_playerMgr.on_client_close(client);

	auto it = s_clientMap.find(client);
	if (it != s_clientMap.end()) {
		free_peer_data(it->second);
		cout << "Debug..s_clientMap_erasing.." << endl;
		s_clientMap.erase(it);
	}

	return true;
}

// 收到网络层发过来的数据
bool OnRecv(uv_tcp_t* client, const char* data, int len)
{
	bool result = false;
	PeerData* peerData = nullptr;

	auto it = s_clientMap.find(client);
	if (it == s_clientMap.end()) {
		goto Exit0;
	}
	peerData = it->second;

	if (sizeof(peerData->recv_buf) - peerData->nowPos < len) {
		goto Exit0;
	}

	memcpy(peerData->recv_buf + peerData->nowPos, data, len);
	peerData->nowPos += len;

	// 完整协议包检测，处理网络粘包
	while (true) {
		int packPos = check_pack(peerData->recv_buf, peerData->nowPos);
		if (packPos < 0) {          // 异常
			goto Exit0;
		}
		else if (packPos > 0) {     // 协议数据完整了
			Packet pack;        // 协议包格式
			// 解码，得到协议号和序列化后的数据
			if (!decode(&pack, peerData->recv_buf, packPos)) {
				goto Exit0;
			}
			// 处理一个数据包
			if (!_OnPackHandle(client, &pack)) {
				goto Exit0;
			}
			if (packPos >= peerData->nowPos) {
				peerData->nowPos = 0;
			}
			else {
				memmove(peerData->recv_buf, peerData->recv_buf + packPos, peerData->nowPos - packPos);
				peerData->nowPos -= packPos;
			}
		}
		else {
			// 没有完全到达完整包
			break;
		}
	}

	result = true;
Exit0:
	return result;
}
//bool OnRecv(uv_tcp_t* client, const char* data, int len) {
//    bool result = false;
//    PeerData* peerData = nullptr;
//    int offset = 0;
//    int remainLen = 0;
//
//    auto it = s_clientMap.find(client);
//    if (it == s_clientMap.end()) {
//        goto Exit0;
//    }
//    peerData = it->second;
//
//    if (sizeof(peerData->recv_buf) - peerData->nowPos < len) {
//        goto Exit0;
//    }
//    memcpy(peerData->recv_buf + peerData->nowPos, data, len);
//    peerData->nowPos += len;
//    remainLen = peerData->nowPos;
//
//    // 完整协议包检测，处理网络粘包
//    while (true) {
//        int packPos = check_pack(peerData->recv_buf + offset, remainLen);
//        if (packPos < 0) {          // 异常
//            goto Exit0;
//        }
//        else if (packPos > 0) {     // 协议数据完整了
//            Packet pack;        // 协议包格式
//            // 解码，得到协议号和序列化后的数据
//            if (!decode(&pack, peerData->recv_buf, packPos)) {
//                goto Exit0;
//            }
//            // 处理一个数据包
//            if (!_OnPackHandle(client, &pack)) {
//                goto Exit0;
//            }
//            if (packPos >= peerData->nowPos) {
//                peerData->nowPos = 0;
//                remainLen = 0;
//            }
//            else {
//                offset += packPos;
//                remainLen -= packPos;
//            }
//        }
//        else {
//            // 没有完全到达完整包
//            break;
//        }
//
//        if (peerData->nowPos > 0) {
//            memmove(peerData->recv_buf, peerData->recv_buf + offset, peerData->nowPos - offset);
//            peerData->nowPos -= offset;
//        }
//    }
//
//    result = true;
//Exit0:
//    return result;
//}

// 处理一个完成的消息包
bool _OnPackHandle(uv_tcp_t* client, Packet* pack)
{
	bool result = false;
	int len = 0;
	// todo 处理收到的数据包
	fprintf(stdout, "OnPackHandle: cmd:%d, len:%d, client:%llu\n", pack->cmd, pack->len, (uint64_t)client);
	switch (pack->cmd) {
	case CLIENT_PING:           // 处理客户端的ping
	{
		fprintf(stdout, "client ping, client:%llu\n", (uint64_t)client);
		len = encode(s_send_buff, SERVER_PONG, nullptr, 0);
		sendData((uv_stream_t*)client, s_send_buff, len);
		break;
	}
	case CLIENT_ADD_REQ:       // 处理客户端发起的一个加法计算请求
	{
		AddReq req;
		req.ParseFromArray(pack->data, pack->len);
		if (!_OnAdd(client, &req)) {
			goto Exit0;
		}
		break;
	}
	case CLIENT_LOGIN_REQ:
	{
		PlayerLoginReq req;
		req.ParseFromArray(pack->data, pack->len);
		g_playerMgr.player_login(client, &req);
		break;
	}
	case CLIENT_CREATE_REQ:
	{
		PlayerCreateReq req;
		req.ParseFromArray(pack->data, pack->len);
		g_playerMgr.player_create(client, &req);
		break;
	}
	case CLIENT_ANNOUNCE_REQ:
	{
		g_playerMgr.announce_request(client);
		break;
	}
	case CLIENT_BUY_ITEM_REQ:
	{
		BuyItemReq req;
		req.ParseFromArray(pack->data, pack->len);
		g_playerMgr.buy_item(client, &req);
		//g_playerMgr.send_backpack_content(client, &req);
	   /* GetBackPackContentReq req;
		req.ParseFromArray(pack->data, pack->len);
		g_playerMgr.send_backpack_content(client, &req);*/
		break;
	}

	case Texus::CLIENT_CMD::CLIENT_JOIN_ROOM_REQ:
	{
		Texus::PlayerTryJoin tryjoin;
		tryjoin.ParseFromArray(pack->data, pack->len);
		cerr << "JOIN REQ: playerid: " << tryjoin.playerid() << " roomid: " << tryjoin.roomid() << '\n';

		User user_join{ client, tryjoin.playerid() ,0 };
		if (g_id.find(user_join.id) == g_id.end()) {
			g_id[user_join.id] = INIT_MONEY;
		}
		user_join.chip = g_id[user_join.id];
		auto room = tryjoin.roomid();

		Texus::PlayerJoinResult joinresult;
		joinresult.set_playerid(user_join.id);
		
		joinresult.set_roomid(room);

		if (room >= g_rooms.size()) {
			joinresult.set_joinresult(Texus::PROTO_RESULT_CODE::JOINROOM_RESULT_FAIL_NO_SUCH_ROOM);
			SendPBToClient(user_join.tcp, Texus::SERVER_CMD::SERVER_JUDGE_JOIN_RSP, &joinresult);
		}
		else if (std::any_of(g_rooms[room].users.begin(), g_rooms[room].users.end(), [&](auto x) {return x.id == user_join.id; })) {
			joinresult.set_joinresult(Texus::PROTO_RESULT_CODE::JOINROOM_RESULT_FAIL_EXISTING_PLAYER_INROOM);
			SendPBToClient(user_join.tcp, Texus::SERVER_CMD::SERVER_JUDGE_JOIN_RSP, &joinresult);
		}
		else if (user_join.chip < MONEY_BAR) {
			joinresult.set_joinresult(Texus::PROTO_RESULT_CODE::JOINROOM_RESULT_FAIL_NO_ENOUGH_MONEY);
			joinresult.set_money(user_join.chip);
			SendPBToClient(user_join.tcp, Texus::SERVER_CMD::SERVER_JUDGE_JOIN_RSP, &joinresult);
		}
		else {
			joinresult.set_joinresult(Texus::PROTO_RESULT_CODE::JOINROOM_RESULT_OK);		
			joinresult.set_money(user_join.chip);

			g_rooms[room].users.push_back(user_join);

			for (int i = 0; i < g_rooms[room].users.size(); ++i) {
				auto new_seattableitem = joinresult.add_seattable();
				new_seattableitem->set_playerid(g_rooms[room].users[i].id);
				new_seattableitem->set_seatnumber(i);
			}
			SendPBToClient(user_join.tcp, Texus::SERVER_CMD::SERVER_JUDGE_JOIN_RSP, &joinresult);

			Texus::BroadcastSeatTable seattable;
			for (int i = 0; i < g_rooms[room].users.size(); ++i) {
				auto new_seattableitem = seattable.add_seattable();
				new_seattableitem->set_playerid(g_rooms[room].users[i].id);
				new_seattableitem->set_seatnumber(i);
			}
			for (const auto& user : g_rooms[room].users) {
				if (user != user_join)
					SendPBToClient(user.tcp, Texus::SERVER_CMD::SERVER_BROADCAST_SEATTABLE, &seattable);
			}
		}
		break;
	}
	case Texus::CLIENT_CMD::CLIENT_QUIT_ROOM_REQ:
	{
		Texus::PlayerTryQuitRoom tryquit;
		tryquit.ParseFromArray(pack->data, pack->len);
		cerr << "QUIT REQ: playerid: " << tryquit.playerid() << " roomid: " << tryquit.roomid() << '\n';

		User user_quit{ client, tryquit.playerid(), 0 };
		auto room = tryquit.roomid();

		Texus::PlayerQuitRoomResult quitresult;
		quitresult.set_playerid(user_quit.id);
		quitresult.set_roomid(room);

		if (room >= g_rooms.size()) {
			quitresult.set_quitresult(Texus::PROTO_RESULT_CODE::QUITROOM_RESULT_FAIL_NO_SUCH_ROOM);
			SendPBToClient(user_quit.tcp, Texus::SERVER_CMD::SERVER_QUITROOM_RSP, &quitresult);
		}
		else if (std::none_of(g_rooms[room].users.begin(), g_rooms[room].users.end(), [&](auto x) {return x == user_quit; })) {
			quitresult.set_quitresult(Texus::PROTO_RESULT_CODE::QUITROOM_RESULT_FAIL_INVALID_ID);
			for (int i = 0; i < g_rooms[room].users.size(); ++i) {
				auto new_seattableitem = quitresult.add_seattable();
				new_seattableitem->set_playerid(g_rooms[room].users[i].id);
				new_seattableitem->set_seatnumber(i);
			}
			SendPBToClient(user_quit.tcp, Texus::SERVER_CMD::SERVER_QUITROOM_RSP, &quitresult);
		}
		else {
			quitresult.set_quitresult(Texus::PROTO_RESULT_CODE::QUITROOM_RESULT_OK);

			auto user_in_room = std::find_if(g_rooms[room].users.begin(), g_rooms[room].users.end(), [&](auto x) {return x == user_quit; });
			if (g_id.find(user_quit.id) == g_id.end()) {
				cerr << "UNEXPECTED CLIENT_QUIT_ROOM_REQ: invalid id\n";
			}
			else {
				g_id[user_quit.id] = user_in_room->chip;
				quitresult.set_money(g_id[user_quit.id]);
			}
			g_rooms[room].users.erase(user_in_room);

			for (int i = 0; i < g_rooms[room].users.size(); ++i) {
				auto new_seattableitem = quitresult.add_seattable();
				new_seattableitem->set_playerid(g_rooms[room].users[i].id);
				new_seattableitem->set_seatnumber(i);
			}
			SendPBToClient(user_quit.tcp, Texus::SERVER_CMD::SERVER_QUITROOM_RSP, &quitresult);

			Texus::BroadcastSeatTable seattable;
			for (int i = 0; i < g_rooms[room].users.size(); ++i) {
				auto new_seattableitem = seattable.add_seattable();
				new_seattableitem->set_playerid(g_rooms[room].users[i].id);
				new_seattableitem->set_seatnumber(i);
			}
			for (const auto& user : g_rooms[room].users) {
				SendPBToClient(user.tcp, Texus::SERVER_CMD::SERVER_BROADCAST_SEATTABLE, &seattable);
			}
		}
		break;
	}
	default:
		fprintf(stderr, "invalid cmd:%d\n", pack->cmd);
		return false;
	}
	result = true;
Exit0:
	return result;
}

bool _OnAdd(uv_tcp_t* client, AddReq* req)
{
	AddRsp rsp;
	rsp.set_result(req->a() + req->b());
	rsp.set_a(req->a());
	rsp.set_b(req->b());
	printf("client request: %d + %d = %d\n", req->a(), req->b(), rsp.result());
	return SendPBToClient(client, SERVER_ADD_RSP, &rsp);
}


