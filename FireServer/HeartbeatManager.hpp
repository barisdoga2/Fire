#pragma once

#include "Server.hpp"
#include "SessionManager.hpp"


class HeartbeatManager : public SessionManager {
public:
    HeartbeatManager(Server* server);

    bool Update(ObjCacheType_t& in_cache, ObjCacheType_t& out_cache, double dt) override;

    bool Receive(ObjCacheType_t& in_cache, ObjCacheType_t& out_cache) override;

    virtual void OnSessionCreate(ObjCacheType_t& out_cache, TickSession* session) override;

    virtual void OnSessionDestroy(ObjCacheType_t& out_cache, TickSession* session, SessionStatus destroyReason) override;

};