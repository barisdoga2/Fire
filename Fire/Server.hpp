#pragma once

#include <thread>
#include <unordered_map>
#include <vector>
#include <array>

#include "Net.hpp"

#define SERVER_STATISTICS

using ROCacheType_t = std::unordered_map<SessionID_t, std::vector<EasySerializeable*>>;

class Session {
public:
    const SessionID_t sessionID;
    const Addr_t addr;
    const Key_t key;
    SequenceID_t sequenceID_in;
    Timestamp_t lastReceive;

    Session(const SessionID_t& sessionID, const Addr_t& addr, const Key_t& key) : sessionID(sessionID), addr(addr), key(key), sequenceID_in(0U), lastReceive(Clock::now())
    {

    }
};

class MainContex {
public:
    std::array<Session*, MAX_SESSIONS> sessions{ nullptr };
    std::mutex sessionsMutex;

    ROCacheType_t readyObjsCache;
    std::mutex readyObjsMutex;

};



struct lua_State;
class EasySocket;
class EasyDB;
class World;
class MainContex;
class EasyBufferManager;
class Server {
public:
    static inline Server* instance;

    // Flags
    bool running;

    // Res
    EasyBufferManager* bf;

    // Net
    EasySocket* sock;
    const unsigned short port;
    std::thread receive;
    std::thread send;
    std::thread update;

    // DB
    EasyDB* db;

    // World
    World* world;

    MainContex* m;

    Server(EasyBufferManager* bf, unsigned short port);
    ~Server();

    void Update();

    void Receive();
    void Send();

    bool Start();
    bool Stop();

    std::string StatsReceive();

    static int Stats(lua_State* L)
    {
        std::cout << instance->StatsReceive();
        return 0;
    }
};