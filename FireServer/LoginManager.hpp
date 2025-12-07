#pragma once

#include "FireServer.hpp"
#include "SessionManager.hpp"


class LoginManager : public SessionManager3 {
public:
    LoginManager(FireServer* server);

    bool Update(ObjCacheType_t& in_cache, ObjCacheType_t& out_cache, double dt) override;

    bool Receive(ObjCacheType_t& in_cache, ObjCacheType_t& out_cache) override;

};