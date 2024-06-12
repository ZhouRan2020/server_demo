#include "utils.h"
#include "../netpack/nethandle.h"
#include "../3rd/src/cJSON/cJSON.h"
#include "filedb/filedb.h"
#pragma region Added Include?
#include "player/player.h"
#pragma endregion

#define LONG_CHAR_BUFF 128

extern uv_loop_t* g_loop;
extern ServerCfg g_config;
extern ItemCfg g_item_config;

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

PeerData* malloc_peer_data() {
    PeerData* peerData = (PeerData*)malloc(sizeof(PeerData));
    if (!peerData)
        return nullptr;

    peerData->nowPos = 0;
    
    return peerData;
}

void free_peer_data(PeerData* peerData) {
    free(peerData);
}

void on_close(uv_handle_t *handle) {
    fprintf(stdout, "client closed:%llu\n", (uint64_t)handle);

    // 通知逻辑层连接关闭
    OnCloseClient((uv_tcp_t*)handle);

    if (handle != NULL) {
        free(handle);
    }
}

void on_write(uv_write_t *req, int status) {
    if (status) {
        fprintf(stderr, "Write error %s, req:%llu\n", uv_strerror(status), (uint64_t)req);
        goto Exit0;
    }
//    fprintf(stdout, "on write, send data to client:%llu success, len:%d\n", (uint64_t)req, ((my_write_t*)req)->buf.len);
Exit0:
    free_write_buff((my_write_t*)req);
}

bool sendData(uv_stream_t* stream, const char* data, int len) {
    bool result = false;
    my_write_t* req = nullptr;
    int uv_write_status;
    req = alloc_write_buff(len);
    if (req == nullptr) {
        goto Exit0;
    }

    memcpy(req->buf.base, data, len);

    //发送buffer数组，第四个参数表示数组大小
    uv_write_status = uv_write(req, stream, &(req->buf), 1, on_write);
    if (uv_write_status != 0) {
        if (uv_write_status == UV_ECANCELED) {
            cout << "UV WRITE DETECTED ERROR OF UV_ECANCELED = -4081" << endl;
            result = false;

        }
        if (uv_write_status == -4047) {
            cout << "UV WRITE DETECTED ERROR OF UV_EPIPE = -4047" << endl;
            result = false;
        }
        goto Exit0;
    }
    req = nullptr;

    result = true;
Exit0:
    if (req) {
        free_write_buff(req);
        req = nullptr;
    }

    if (!result) {
        if (uv_is_closing(reinterpret_cast<uv_handle_t*>(stream)) != 0)//猜测疑为重复close同一个stream
        {
            cout << "stream has already been closed!" << endl;
        }
        else {
            uv_close((uv_handle_t*)stream, on_close);
        }
    }

    return result;
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
        goto Exit0;
    }
    // 调用逻辑层消息处理
    if (!OnRecv((uv_tcp_t*)stream, buf->base, (int)nread)) {
        //changed
        if (!uv_is_closing((uv_handle_t*)stream)) {
            uv_close((uv_handle_t*)stream, on_close);
        }
        
        goto Exit0;
    }
Exit0:
    free_read_buff(buf);
}

void on_new_connection(uv_stream_t *server, int status) {
    uv_tcp_t *client = nullptr;
    if (status < 0) {
        fprintf(stderr, "New connection error %s\n", uv_strerror(status));
        goto Exit0;
    }
    client = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));

    uv_tcp_init(g_loop, client);

    if (uv_accept(server, (uv_stream_t*)client) == 0) {
        fprintf(stdout, "accept client: %llu\n", (uint64_t)client);

        // 通知逻辑层新连接建立
        if (!OnNewClient(client)) {
            goto Exit0;
        }

        uv_read_start((uv_stream_t*)client, alloc_read_buff, on_read);
        client = nullptr;
    }
    else {
        fprintf(stderr, "accept failed\n");
    }
Exit0:
    if (client) {
        uv_close((uv_handle_t*)client, NULL);
        free(client);
        client = nullptr;
    }
    return;
}

int _parseCfg(const char * const monitor) { //解析config，即把本地的配置json文件读取到内存中
    const cJSON *resolution = NULL;
    const cJSON *resolutions = NULL;
    const cJSON *temp = NULL;
    const cJSON *ip = NULL;
    const cJSON *port = NULL;

    int status = 0;
    cJSON *monitor_json = cJSON_Parse(monitor);
    if (monitor_json == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }
        status = 0;
        goto end;
    }

    temp = cJSON_GetObjectItemCaseSensitive(monitor_json, "gameserver");
    if (!cJSON_IsObject(temp)) {
        fprintf(stderr, "invalid config, gameserver field must object\n");
        goto end;
    }

    ip = cJSON_GetObjectItemCaseSensitive(temp, "ip");
    if (!cJSON_IsString(ip)) {
        fprintf(stderr, "invalid gameserver config, ip field is not string\n");
        goto end;
    }
    strcpy(g_config.game_server_ip, ip->valuestring);

    port = cJSON_GetObjectItemCaseSensitive(temp, "port");
    if (!cJSON_IsNumber(port)) {
        fprintf(stderr, "invalid gameserver config, port field is not number\n");
        goto end;
    }
    g_config.game_server_port = port->valueint;

    temp = cJSON_GetObjectItemCaseSensitive(monitor_json, "announce");
    if (cJSON_IsString(temp))
        g_config.default_announce = temp->valuestring;
end:
    cJSON_Delete(monitor_json);
    return status;
}
int _parseItemCfg(const char* const monitor) {
    int status = 0;
    const cJSON* name = NULL;
    const cJSON* introduce = NULL;
    const cJSON* price = NULL;
    const cJSON* iconName = NULL;
    const cJSON* itemid = NULL;
    int arraySize = 0;
#pragma region parse from monitor
    cJSON* items = cJSON_Parse(monitor);
    cJSON* parsed_item_array = cJSON_GetObjectItem(items, "items");
    if (parsed_item_array == NULL) {
        const char* error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }
        status = 0;
        goto EXIT0;
    }
#pragma endregion
    arraySize = cJSON_GetArraySize(parsed_item_array);
    
    for (int i = 0; i < arraySize; i++) {
        auto arrayItem = cJSON_GetArrayItem(parsed_item_array, i);
        name = cJSON_GetObjectItemCaseSensitive(arrayItem, "name");
        introduce = cJSON_GetObjectItemCaseSensitive(arrayItem, "introduce");
        price = cJSON_GetObjectItemCaseSensitive(arrayItem, "price");
        iconName = cJSON_GetObjectItemCaseSensitive(arrayItem, "iconName");
        itemid = cJSON_GetObjectItemCaseSensitive(arrayItem, "itemID");

        if (name && introduce && price && iconName && itemid) {
            int32_t itemID = cJSON_GetNumberValue(itemid);
            ItemInCfg toInsertItemInCfg;
            toInsertItemInCfg.name = cJSON_GetStringValue(name);
            toInsertItemInCfg.introduce = cJSON_GetStringValue(introduce);
            toInsertItemInCfg.price = cJSON_GetNumberValue(price);
            toInsertItemInCfg.iconName = cJSON_GetStringValue(iconName);
            g_item_config.ItemDictionaryByItemId.insert(pair<int32_t, ItemInCfg>(itemID, toInsertItemInCfg));
        }
        else {
            fprintf(stderr, "invalid item config\n");
            status = -1;
            goto EXIT0;
        }
    }
 
EXIT0:
    cJSON_Delete(parsed_item_array);
    return status;
}
int loadItemConfig() {
    const char* path = "config/ItemConfig.json";
    int len = 0;
    len = readCfg(path, nullptr, 0);
    if (len < 0) {
        fprintf(stderr, "can't find %s\n", path);
        return -1;
    }
    char* temp = (char*)malloc(len);
    readCfg(path, temp, len);
    _parseItemCfg(temp);
    free(temp);
    return 0;
}
int loadConfig() {
    const char* path = "config/config.json";
    char* temp = nullptr;
    int len = 0;
    len = readCfg(path, nullptr, 0);
    if (len < 0) {
        fprintf(stderr, "can't find %s\n", path);
        return -1;
    }
    temp = (char*)malloc(len);
    readCfg(path, temp, len);

    _parseCfg(temp);

    free(temp);

    return 0;
}

int init_socket_server(const char* ip, uint16_t port, uv_loop_t* loop) {
    int result = 0;
    sockaddr_in addr;
    // todo 没地方释放了
    uv_tcp_t* server = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));

    result = uv_tcp_init(loop, server);
    if (result != 0) {
        fprintf(stderr, "uv_tcp_init error %s\n", uv_strerror(result));
        goto Exit0;
    }

    result = uv_ip4_addr(ip, port, &addr);
    if (result != 0) {
        fprintf(stderr, "uv_ip4_addr error %s\n", uv_strerror(result));
        goto Exit0;
    }

    result = uv_tcp_bind(server, (const struct sockaddr*)&addr, 0);
    if (result != 0) {
        fprintf(stderr, "uv_tcp_bind error %s\n", uv_strerror(result));
        goto Exit0;
    }

    result = uv_listen((uv_stream_t*)server, SOMAXCONN, on_new_connection);
    if (result != 0) {
        fprintf(stderr, "uv_listen error %s\n", uv_strerror(result));
        goto Exit0;
    }
    result = 0;
Exit0:
    return result;
}


std::string TransformStructToJsonStr(Player p) {
    cJSON* playerObject = cJSON_CreateObject();
    cJSON_AddStringToObject(playerObject, "Name", p.Name.c_str());
    cJSON_AddStringToObject(playerObject, "Password", p.Password.c_str());
    cJSON_AddStringToObject(playerObject, "PlayerID", p.PlayerID.c_str());
    cJSON_AddNumberToObject(playerObject, "Speed", p.Speed);
    cJSON_AddNumberToObject(playerObject, "money", p.money);
#pragma region  add Position
    cJSON* position = cJSON_CreateObject();
    cJSON_AddNumberToObject(position, "x", p.Position.x());
    cJSON_AddNumberToObject(position, "y", p.Position.y());
    cJSON_AddNumberToObject(position, "z", p.Position.z());
    cJSON_AddItemToObject(playerObject, "Position", position);
#pragma endregion
#pragma region add Rotation
    cJSON* rotation = cJSON_CreateObject();
    cJSON_AddNumberToObject(rotation, "x", p.Rotation.x());
    cJSON_AddNumberToObject(rotation, "y", p.Rotation.y());
    cJSON_AddNumberToObject(rotation, "z", p.Rotation.z());
    cJSON_AddNumberToObject(rotation, "w", p.Rotation.w());
    cJSON_AddItemToObject(playerObject, "Rotation", rotation);
#pragma endregion
#pragma region add Items
    cJSON* itemsArray = cJSON_CreateArray();
    for (auto playerItem : p.playerItems) {
        cJSON* json_item = cJSON_CreateObject();
        cJSON_AddNumberToObject(json_item, "ItemID", playerItem.itemid());
        cJSON_AddNumberToObject(json_item, "Price", playerItem.price());
        cJSON_AddNumberToObject(json_item, "HeapSize", playerItem.heapsize());
        cJSON_AddItemToArray(itemsArray, json_item);
    }

    cJSON_AddItemToObject(playerObject, "playerItems", itemsArray);
        /*
        string PlayerID;
    string Password;
    string Name;
    Float3 Position;
    Float4 Rotation;
    float Speed;

    vector<Item> playerItems;
    int money;
        */
#pragma endregion

    string output = cJSON_Print(playerObject);
    cJSON_Delete(playerObject);
    return output;
}

Player TransformJsonStrToStruct(string jsonstr) {
    Player p;
    cJSON* object = cJSON_Parse(jsonstr.c_str());
    if (object == NULL) {
        fprintf(stderr, "json string Parse error");
    }
#pragma region writename,password,playerID,speed
    cJSON* name = cJSON_GetObjectItemCaseSensitive(object, "Name");
    p.Name = cJSON_GetStringValue(name);
    cJSON* password = cJSON_GetObjectItemCaseSensitive(object, "Password");
    p.Password = cJSON_GetStringValue(password);
    cJSON* playerID = cJSON_GetObjectItemCaseSensitive(object, "PlayerID");
    p.PlayerID = cJSON_GetStringValue(playerID);
    cJSON* speed = cJSON_GetObjectItemCaseSensitive(object, "Speed");
    p.Speed = cJSON_GetNumberValue(speed);
#pragma endregion

#pragma region write position&rotation
    cJSON* position = cJSON_GetObjectItemCaseSensitive(object, "Position");
    p.Rotation.set_x(cJSON_GetNumberValue(cJSON_GetObjectItemCaseSensitive(position, "x")));
    p.Rotation.set_y(cJSON_GetNumberValue(cJSON_GetObjectItemCaseSensitive(position, "y")));
    p.Rotation.set_z(cJSON_GetNumberValue(cJSON_GetObjectItemCaseSensitive(position, "z")));

    cJSON* rotation = cJSON_GetObjectItemCaseSensitive(object, "Rotation");
    p.Rotation.set_x(cJSON_GetNumberValue(cJSON_GetObjectItemCaseSensitive(rotation, "x")));
    p.Rotation.set_y(cJSON_GetNumberValue(cJSON_GetObjectItemCaseSensitive(rotation, "y")));
    p.Rotation.set_z(cJSON_GetNumberValue(cJSON_GetObjectItemCaseSensitive(rotation, "z")));
    p.Rotation.set_w(cJSON_GetNumberValue(cJSON_GetObjectItemCaseSensitive(rotation, "w")));
#pragma endregion

#pragma region writeItems&money
    cJSON* itemsArray = cJSON_GetObjectItemCaseSensitive(object, "playerItems");
    if (itemsArray == nullptr || !cJSON_IsArray(itemsArray)) {
        // Handle invalid or missing "myvec" array
        cout << "Handle invalid or missing itemarray array" << endl;
        cJSON_Delete(object);
        return p;
    }
    int arraySize = cJSON_GetArraySize(itemsArray);
    for (int i = 0; i < arraySize; i++) {
        cJSON* json_item = cJSON_GetArrayItem(itemsArray, i);
        Item generatedItem = Item();
        generatedItem.set_itemid(cJSON_GetNumberValue(cJSON_GetObjectItemCaseSensitive(json_item,"ItemID")));
        generatedItem.set_price(0);
        generatedItem.set_heapsize(0);
        p.playerItems.push_back(generatedItem);
    }//写好了vector<Item> playerItems;

    p.money = cJSON_GetNumberValue(cJSON_GetObjectItemCaseSensitive(object, "money"));
#pragma endregion

    cJSON_Delete(object);
    return p;
}//coded well ,not tested

std::string TransformStructToJsonStr(ItemInCfg item) {
    cJSON* itemObject = cJSON_CreateObject();
    cJSON_AddStringToObject(itemObject, "Introduce", item.introduce.c_str());
    cJSON_AddStringToObject(itemObject, "Name", item.name.c_str());
    cJSON_AddStringToObject(itemObject, "IconName", item.iconName.c_str());
    cJSON_AddNumberToObject(itemObject, "Price", item.price);

    string output = cJSON_Print(itemObject);
    cJSON_Delete(itemObject);
    return output;
}
//ItemInCfg TransformJsonStrToStruct_ItemCfg(string jsonstr) {
//    //从json到内存读的是配置文件，不是message Item！
//    ItemInCfg item;
//    cJSON* object = cJSON_Parse(jsonstr.c_str());
//
//    cJSON* introduce = cJSON_GetObjectItemCaseSensitive(object, "Introduce");
//    item.set_introduce(cJSON_GetStringValue(introduce));
//    cJSON* name = cJSON_GetObjectItemCaseSensitive(object, "Name");
//    item.set_name(cJSON_GetStringValue(name));
//    cJSON* iconname = cJSON_GetObjectItemCaseSensitive(object, "IconName");
//    item.set_iconname(cJSON_GetStringValue(iconname));
//
//    cJSON* heapsize = cJSON_GetObjectItemCaseSensitive(object, "HeapSize");
//    item.set_heapsize(cJSON_GetNumberValue(iconname));
//    cJSON* itemid = cJSON_GetObjectItemCaseSensitive(object, "ItemID");
//    item.set_itemid(cJSON_GetNumberValue(itemid));
//    cJSON* price = cJSON_GetObjectItemCaseSensitive(object, "Price");
//    item.set_price(cJSON_GetNumberValue(price));
//
//    if (object == NULL) {
//        fprintf(stderr, "json string Parse error");
//    }
//    cJSON_Delete(object);
//    return item;
//}


