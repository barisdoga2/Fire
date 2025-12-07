#pragma once

#include "FireServer.hpp"
#include "SessionManager.hpp"

class ChatManager : public SessionManager3 {
public:
    ChatManager(FireServer* server);

    bool Update(ObjCacheType_t& in_cache, ObjCacheType_t& out_cache, double dt) override;

    bool Receive(ObjCacheType_t& in_cache, ObjCacheType_t& out_cache) override;

    virtual void OnSessionCreate(ObjCacheType_t& out_cache, FireSession* session) override;

    virtual void OnSessionDestroy(ObjCacheType_t& out_cache, FireSession* session, SessionStatus destroyReason) override;

};