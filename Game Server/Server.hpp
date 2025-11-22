#pragma once

#include "../Fire/EasyServer.hpp"

class Server : public EasyServer {
public:
	// World
	World* world;

	Server(EasyBufferManager* bf, unsigned short port);
	bool Start();

	void Tick(double _dt) override;
	void OnInit() override;
	void OnDestroy() override;
	bool OnSessionCreate(Session* session) override;
	void OnSessionDestroy(Session* session, SessionStatus disconnectReason) override;
	void DoProcess(ObjCacheType_t& in_cache, ObjCacheType_t& out_cache) override;

};