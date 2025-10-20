#pragma once

#include <thread>
#include <unordered_map>
#include <vector>
#include <array>

#include "Net.hpp"




using ObjCacheType_t = std::unordered_map<SessionID_t, std::vector<EasySerializeable*>>;

template <class T>
class LockedVec_t {
public:
    std::vector<T> vec;
    std::mutex mutex;
};

class Session {
public:
    const SessionID_t sessionID;
    const Addr_t addr;
    const Key_t key;
    SequenceID_t sequenceID_in;
    SequenceID_t sequenceID_out;
    Timestamp_t lastReceive;

    Session(const SessionID_t& sessionID, const Addr_t& addr, const Key_t& key, const SequenceID_t& sequenceID_in, const SequenceID_t& sequenceID_out) : sessionID(sessionID), addr(addr), key(key), sequenceID_in(sequenceID_in), sequenceID_out(sequenceID_out), lastReceive(Clock::now())
    {

    }
};

class MainContex {
public:
    class ObjCache_t {
    public:
        ObjCacheType_t cache;
        std::mutex mutex;
    };

    std::array<Session*, MAX_SESSIONS> sessions{ nullptr };
    std::mutex sessionsMutex;

    ObjCache_t in_cache;
    ObjCache_t out_cache;

};



struct lua_State;
class EasySocket;
class EasyDB;
class World;
class MainContex;
class EasyBufferManager;
class Server {
private:
    static inline Server* instance = nullptr;

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

public:
    MainContex* m;

    Server(EasyBufferManager* bf, unsigned short port);
    ~Server();

    bool Start();
    bool Stop();
    bool IsRunning();

    bool CreateSession(Session* session);

    static int Stats(lua_State* L);

private:
    void Update();
    void Receive();
    void Send();

    std::string StatsReceive();
    std::string StatsSend();
    std::string StatsUpdate();

};