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
	void OnSessionCreate(Session* session) override;
	void OnSessionDestroy(Session* session) override;
	void DoProcess(ObjCacheType_t& in_cache, ObjCacheType_t& out_cache) override;

};