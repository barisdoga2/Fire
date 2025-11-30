#pragma once

#include <EasyServer.hpp>
#include "Managers.hpp"

class HeartbeatManager : public SessionManager {
public:
    HeartbeatManager();

    bool Update(ObjCacheType_t& out_cache, double dt) override;

    bool Receive(EasyServer* server, ObjCacheType_t& in_cache, ObjCacheType_t& out_cache) override;

    virtual void OnSessionCreate(Session* session) override;

    virtual void OnSessionDestroy(Session* session, SessionStatus destroyReason) override;

};