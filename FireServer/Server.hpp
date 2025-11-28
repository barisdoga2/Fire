#pragma once

#include <EasyServer.hpp>

class UserStats;
class SessionManager;
class UD {
public:
    Session* session{};
    UserStats* stats{};
    std::vector<SessionManager*> managers{};
    UD(const UD& ud) : session(ud.session), stats(ud.stats), managers(ud.managers)
    {

    }
    UD(Session* session = nullptr, UserStats* stats = nullptr, const std::vector<SessionManager*>& managers = {}) : session(session), stats(stats), managers(managers)
    {

    }
};


#define LOGIN_MANAGER 1U
class LoginManager : public SessionManager {
public:
    LoginManager() : SessionManager(LOGIN_MANAGER)
    {

    }

    bool Update(ObjCacheType_t& out_cache, double dt) override;

    bool Receive(EasyDB* db, ObjCacheType_t& in_cache, ObjCacheType_t& out_cache) override;

};

#define HEARTBEAT_MANAGER 2U
class HeartbeatManager : public SessionManager {
public:
    HeartbeatManager() : SessionManager(HEARTBEAT_MANAGER)
    {

    }

    bool Update(ObjCacheType_t& out_cache, double dt) override;

    bool Receive(EasyDB* db, ObjCacheType_t& in_cache, ObjCacheType_t& out_cache) override;

};

class Server : public EasyServer {
public:
    static inline std::unordered_map<unsigned int, SessionManager*> managers = { 
        {LOGIN_MANAGER, new LoginManager()},
        {HEARTBEAT_MANAGER, new HeartbeatManager()}
    };

	// World
	World* world;

	Server(EasyBufferManager* bf, unsigned short port);
	~Server();
	bool Start();

	void Tick(double _dt) override;
	void OnInit() override;
	void OnDestroy() override;
	bool OnSessionCreate(Session* session) override;
	void OnSessionDestroy(Session* session, SessionStatus disconnectReason) override;
	void DoProcess(ObjCacheType_t& in_cache, ObjCacheType_t& out_cache) override;

};