#pragma once

#include "EasyNet.hpp"

class EasySerializeable;
class EasyBuffer;
using ServerCache_t = std::unordered_map<SessionID_t, std::vector<EasySerializeable*>>;

class SessionBase {
public:
    SessionID_t* sid;
    Addr_t* addr;
    Key_t* key;
    Key_Expt_t* keyExpr;
    Timestamp_t* recv;
};

class ServerCallback {
public:
    virtual bool OnServerStart() = 0U;
    virtual void OnServerStop(std::string shutdownMessage) = 0U;

    virtual bool OnSessionKeyExpiry(const SessionBase& base) = 0U;
    virtual bool OnSessionReconnect(const SessionBase& base, const SessionBase& reconnectingBase) = 0U;
    virtual bool OnSessionCreate(const SessionBase& base, EasyBuffer* buffer) = 0U;
    virtual void OnSessionDestroy(const SessionBase& base, std::string disconnectMessage = "") = 0U;

};

class ServerContext;
class EasyBufferManager;
class BaseServer {
    static inline BaseServer* instance = nullptr;

    bool running;

    ServerContext* cntx;

public:
    BaseServer();

    ~BaseServer();

    std::vector<EasySerializeable*> Process(const CryptData& crypt, EasyBuffer* buffer);

    void Send(SessionID_t sid, const std::vector<EasySerializeable*>& objs, const Addr_t& addr, const SequenceID_t& seqid_in, const SequenceID_t& seqid_out, const Key_t& key);

    void Send(SessionID_t sid, const std::vector<EasySerializeable*>& objs);

    void DestroySession(SessionID_t sid, std::string disconnectMessage = "");

    bool Start(EasyBufferManager* bm, const unsigned short port, ServerCallback* cbk, bool encryption = true, bool compression = true);

    void Stop(std::string shutdownMessage = "");

    virtual void Update(double dt);

    bool IsRunning();

protected:
    ServerCache_t& GetReceiveCache();
    ServerCache_t& GetSendCache();

};
