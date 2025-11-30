#pragma once

#include "Server.hpp"
#include "SessionManager.hpp"


class LoginManager : public SessionManager {
public:
    LoginManager(Server* server);

    bool Update(ObjCacheType_t& in_cache, ObjCacheType_t& out_cache, double dt) override;

    bool Receive(ObjCacheType_t& in_cache, ObjCacheType_t& out_cache) override;

};