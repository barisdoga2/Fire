#pragma once
#include <thread>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <array>

#include "EasyNet.hpp"









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
    else if (status == SERVER_TIMED_OUT)
        str = "SERVER_TIMED_OUT";
    else if (status == SEQUENCE_MISMATCH)
        str = "SEQUENCE_MISMATCH";
    else if (status == RECONNECTED)
        str = "RECONNECTED";
    else if (status == CLIENT_LOGGED_OUT)
        str = "CLIENT_LOGGED_OUT";
    else
        str = "UNKNOWN";
    return str;
}

template <class T>
class LockedVec_t {
public:
    std::vector<T> vec;
    std::mutex mutex;
};

class Session {
public:
    const SessionID_t sessionID;
    const UserID_t userID;
    const Addr_t addr;
    const Key_t key;
    SequenceID_t sequenceID_in;
    SequenceID_t sequenceID_out;
    Timestamp_t lastReceive;

    Session() = delete;
    Session(const Session& session) = delete;

    Session(const SessionID_t& sessionID, const UserID_t& userID, const Addr_t& addr, 
        const Key_t& key, const SequenceID_t& sequenceID_in, const SequenceID_t& sequenceID_out, const Timestamp_t& lastReceive)
        : sessionID(sessionID), userID(userID), addr(addr), key(key), sequenceID_in(sequenceID_in), sequenceID_out(sequenceID_out), lastReceive(lastReceive)
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

    std::vector<SessionID_t> sessionIDs;
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


    // Context
    EasyBufferManager* bf;

public:
    // DB
    EasyDB* db;


    EasyServer(EasyBufferManager* bf, unsigned short port);
    ~EasyServer();

    bool Start();
    bool Stop();
    bool IsRunning();

    static int Stats(lua_State* L);
    void Broadcast(const std::vector<EasySerializeable*>& cache);
    bool SendInstantPacket(Session* destination, const std::vector<EasySerializeable*>& objs);
    bool DestroySession_external(SessionID_t session, SessionStatus disconnectReason);
    bool DestroySession_internal(Session* session, SessionStatus disconnectReason);

private:
    void Update();
    void Receive();
    void Send();

    bool CreateSession_internal(Session* session);

    std::string StatsReceive();
    std::string StatsSend();
    std::string StatsUpdate();

protected:
    MainContex* m;

    virtual void Tick(double _dt) = 0U;
    virtual void OnInit() = 0U;
    virtual void OnDestroy() = 0U;
    virtual bool OnSessionCreate(Session* session) = 0U;
    virtual void OnSessionDestroy(Session* session, SessionStatus disconnectStatus) = 0U;
    virtual void DoProcess(ObjCacheType_t& in_cache, ObjCacheType_t& out_cache) = 0U;

    friend class EasyServer_ReceviveHelper;
    friend class EasyServer_SendHelper;
    friend class EasyServer_UpdateHelper;
};
