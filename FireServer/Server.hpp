#pragma once

#include <EasyServer.hpp>
#include "Net.hpp"
#include "SessionManager.hpp"

enum SessionManagers : unsigned int {
	LOGIN_MANAGER = 0U,
	HEARTBEAT_MANAGER = 1U,
	CHAT_MANAGER = 2U,
	PLAYER_MANAGER = 3U,
};






class Server;
class TickSession {
	std::unordered_map<SessionManagers, SessionManager*> managers{};
public:
	static inline const uint32_t sessionTimeout{ 10000 };

	const SessionID_t sessionID;
	const UserID_t userID;
	const Addr_t addr;

	std::string username;

	Timestamp_t lastReceive{};
	bool logoutRequested{};

	UserStats stats{};

	SessionStatus status = UNSET;

	unsigned long long moveTimestamp{};
	glm::vec3 position{}, rotation{}, direction{};

	TickSession(Session* session);

	void Destroy(ObjCacheType_t& out_cache, SessionStatus status);

	void RegisterToManager(ObjCacheType_t& out_cache, SessionManagers manager);
};



class Server : public EasyServer {
public:
	std::mutex sessionsMutex;
	std::unordered_map<SessionID_t, TickSession*> sessions;

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