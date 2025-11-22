#pragma once

#include <thread>
#include <unordered_map>
#include <vector>
#include <array>

#include "EasyNet.hpp"
#include "EasyScheduler.hpp"
#include "EasySerializer.hpp"






struct UserStats {
    unsigned int gametime{};
    unsigned int golds{};
    unsigned int diamonds{};
    bool tutorial_done{};
    std::string characters_owned{};
};

enum SessionStatus {
    UNSET,
    CONNECTING,
    CONNECTED,

    ADDR_MISMATCH,
    CRYPT_ERR,
    TIMED_OUT,
    SEQUENCE_MISMATCH,
    RECONNECTED,
};

inline static std::string SessionStatus_Str(const SessionStatus& status)
{
    std::string str;
    if (status == UNSET)
        str = "UNSET";
    else if (status == CONNECTING)
        str = "CONNECTING";
    else if (status == CONNECTED)
        str = "CONNECTED";
    else if (status == ADDR_MISMATCH)
        str = "Connected from another client!";
    else if (status == CRYPT_ERR)
        str = "CRYPT_ERR";
    else if (status == TIMED_OUT)
        str = "TIMED_OUT";
    else if (status == SEQUENCE_MISMATCH)
        str = "SEQUENCE_MISMATCH";
    else if (status == RECONNECTED)
        str = "RECONNECTED";
    else
        str = "UNKNOWN";
    return str;
}



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

    UserStats stats;

    Session(const SessionID_t& sessionID, const Addr_t& addr, const Key_t& key, const SequenceID_t& sequenceID_in, const SequenceID_t& sequenceID_out, const UserStats& stats) : sessionID(sessionID), addr(addr), key(key), sequenceID_in(sequenceID_in), sequenceID_out(sequenceID_out), lastReceive(Clock::now()), stats(stats)
    {

    }

    ~Session()
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

    bool SendInstantPacket(Session* destination, const std::vector<EasySerializeable*>& objs);

    bool CreateSession_internal(Session* session);
    bool DestroySession_internal(SessionID_t sessionID, SessionStatus disconnectReason);

private:
    void Update();
    void Receive();
    void Send();



    std::string StatsReceive();
    std::string StatsSend();
    std::string StatsUpdate();

protected:
    bool DestroySession(SessionID_t sessionID, SessionStatus disconnectReason);
    bool DestroySession(Session* session, SessionStatus disconnectReason);
    Session* GetSession(SessionID_t sessionID);
    bool SetSessionTimeout(uint32_t ms);

    virtual void Tick(double _dt) = 0U;
    virtual void OnInit() = 0U;
    virtual void OnDestroy() = 0U;
    virtual bool OnSessionCreate(Session* session) = 0U;
    virtual void OnSessionDestroy(Session* session, SessionStatus disconnectStatus) = 0U;
    virtual void DoProcess(ObjCacheType_t& in_cache, ObjCacheType_t& out_cache) = 0U;
};
