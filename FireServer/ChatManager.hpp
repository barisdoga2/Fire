#pragma once

#include "Server.hpp"
#include "SessionManager.hpp"

class ChatManager : public SessionManager {
public:
    ChatManager(Server* server);

    bool Update(ObjCacheType_t& out_cache, double dt) override;

    bool Receive(ObjCacheType_t& in_cache, ObjCacheType_t& out_cache) override;

    virtual void OnSessionCreate(TickSession* session) override;

    virtual void OnSessionDestroy(TickSession* session, SessionStatus destroyReason) override;

};