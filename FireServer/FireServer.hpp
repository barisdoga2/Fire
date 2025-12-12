#pragma once

#include <algorithm>
#include <iostream>
#include <unordered_map>
#include "SqLite3.hpp"
#include "ServerNet.hpp"
#include <BaseServer.hpp>

class FireServer : public BaseServer, ServerCallback {
	class FireSession {
	public:
		std::string username;
		SessionID_t sid;
		UserID_t uid;
		Addr_t addr;
		UserStats stats;
		Timestamp_t recv;

		bool logoutRequested;

		FireSession(std::string username, SessionID_t sid, UserID_t uid, Addr_t addr, UserStats stats, Timestamp_t recv);

	};
	std::unordered_map<SessionID_t, FireSession*> sessions;
	
	SqLite3 sqlite;

public:
	FireServer(EasyBufferManager* bm, unsigned short port);
	~FireServer();

	void Update(double dt) override;

	void Broadcast(std::string broadcastMessage);

	bool OnServerStart() override;
	void OnServerStop() override;

	bool OnSessionCreate(const SessionBase& base) override;
	void OnSessionDestroy(const SessionBase& base) override;
	
	bool OnSessionKeyExpiry(const SessionBase& base) override;
	bool OnSessionReconnect(const SessionBase& base, const SessionBase& reconnectingBase) override;

	friend class ServerUI;
};

