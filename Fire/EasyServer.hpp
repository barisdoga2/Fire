#pragma once

#include <thread>
#include <unordered_map>
#include <vector>
#include <array>

#include "Net.hpp"
#include "EasyScheduler.hpp"




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
class EasyServer {
private:
    static inline EasyServer* instance = nullptr;

    // Flags
    bool running;

    // Net
    EasySocket* sock;
    const unsigned short port;
    std::thread receive;
    std::thread send;
    std::thread update;

    // DB
    EasyDB* db;

public:
    // Context
    MainContex* m;
    uint32_t sessionTimeout;
    EasyBufferManager* bf;

    EasyServer(EasyBufferManager* bf, unsigned short port);
    ~EasyServer();

    bool Start();
    bool Stop();
    bool IsRunning();

    static int Stats(lua_State* L);

    bool CreateSession_internal(Session* session);
    bool DestroySession_internal(SessionID_t sessionID);

private:
    void Update();
    void Receive();
    void Send();



    std::string StatsReceive();
    std::string StatsSend();
    std::string StatsUpdate();

protected:
    bool DestroySession(SessionID_t sessionID);
    bool DestroySession(Session* session);
    Session* GetSession(SessionID_t sessionID);
    bool SetSessionTimeout(uint32_t ms);

    virtual void Tick(double _dt) = 0U;
    virtual void OnInit() = 0U;
    virtual void OnDestroy() = 0U;
    virtual void OnSessionCreate(Session* session) = 0U;
    virtual void OnSessionDestroy(Session* session) = 0U;

};
