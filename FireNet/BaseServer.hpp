#pragma once

#include "FireNet.hpp"

class EasySerializeable;
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
    virtual void OnServerStop() = 0U;

    virtual bool OnSessionKeyExpiry(const SessionBase& base) = 0U;
    virtual bool OnSessionReconnect(const SessionBase& base, const SessionBase& reconnectingBase) = 0U;
    virtual bool OnSessionCreate(const SessionBase& base) = 0U;
    virtual void OnSessionDestroy(const SessionBase& base) = 0U;

};

class ServerContext;
class EasyBufferManager;
class BaseServer {
    static inline BaseServer* instance = nullptr;

    const unsigned short port;

    bool running;

    ServerContext* cntx;

public:
    BaseServer(EasyBufferManager* bm, const unsigned short port, ServerCallback* cbk, bool encryption = true, bool compression = true);

    ~BaseServer();

    bool Start();

    void Stop();

    void Update();

    bool IsRunning();

protected:
    ServerCache_t& GetReceiveCache();
    ServerCache_t& GetSendCache();

};
