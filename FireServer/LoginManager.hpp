#pragma once

#include <EasyServer.hpp>
#include "Managers.hpp"

class LoginManager : public SessionManager {
public:
    LoginManager();

    bool Update(ObjCacheType_t& out_cache, double dt) override;

    bool Receive(EasyServer* server, ObjCacheType_t& in_cache, ObjCacheType_t& out_cache) override;

};