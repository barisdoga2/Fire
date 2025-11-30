#pragma once

#include <EasyServer.hpp>
#include "HeartbeatManager.hpp"
#include "LoginManager.hpp"




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