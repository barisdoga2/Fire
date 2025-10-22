#pragma once

#include "EasyServer.hpp"

class Server : public EasyServer {
public:
	// World
	World* world;

	Server(EasyBufferManager* bf, unsigned short port);

	void Tick(double _dt) override;
	void OnInit() override;
	void OnDestroy() override;
	void OnSessionCreate(Session* session) override;
	void OnSessionDestroy(Session* session) override;

};