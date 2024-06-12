#pragma once


struct User {
    uv_tcp_t* tcp;
    string id;
    bool operator==(User const& other)
    {
        return tcp == other.tcp && id == other.id;
    }
};

struct Room {
    static const int user_lim = 10;
    vector<User> users;

};


