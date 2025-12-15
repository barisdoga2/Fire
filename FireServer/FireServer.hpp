#pragma once

#include <algorithm>
#include <iostream>
#include <unordered_map>
#include "SqLite3.hpp"
#include "ServerNet.hpp"
#include <BaseServer.hpp>

class FireServer : public BaseServer, ServerCallback {
	std::unordered_map<SessionID_t, FireSession*> sessions;
	
	SqLite3 sqlite;

public:
	FireServer();
	~FireServer();

	bool Start(EasyBufferManager* bm);
	void Stop(std::string shutdownMessage = "");

	void Update(double dt) override;

	void BroadcastMessage(std::string broadcastMessage);

	bool OnServerStart() override;
	void OnServerStop(std::string shutdownMessage) override;

	bool OnSessionCreate(const SessionBase& base, EasyBuffer* buffer) override;
	void OnSessionDestroy(const SessionBase& base, std::string disconnectMessage = "") override;
	
	bool OnSessionKeyExpiry(const SessionBase& base) override;
	bool OnSessionReconnect(const SessionBase& base, const SessionBase& reconnectingBase) override;

	friend class ServerUI;
};

