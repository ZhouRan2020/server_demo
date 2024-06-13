#pragma once
constexpr auto INIT_MONEY = 1000000;
constexpr auto MONEY_BAR = 100000;
struct User {
    uv_tcp_t* tcp;
    string id;
    int chip=-1;
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


