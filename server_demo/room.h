#pragma once

struct User {
    uv_tcp_t* tcp;
    string id;
    bool operator==(User const& other) const
    {
        return tcp == other.tcp && id == other.id;
    }
    bool operator!=(User const& other) const
    {
        return !(*this==other);
    }
};

struct Room {
    static const int user_lim = 10;
    vector<User> users;
};


