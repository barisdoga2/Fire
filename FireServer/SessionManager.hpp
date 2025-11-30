#pragma once

#include <EasyNet.hpp>

class Server;
class TickSession;
class SessionManager {
public:
	Server* server;
	unsigned int managerID;

	SessionManager(Server* server, unsigned int managerID) : server(server), managerID(managerID)
	{

	}

	virtual bool Update(ObjCacheType_t& in_cache, ObjCacheType_t& out_cache, double dt) = 0U;
	virtual bool Receive(ObjCacheType_t& in_cache, ObjCacheType_t& out_cache) = 0U;
	virtual void OnSessionCreate(ObjCacheType_t& out_cache, TickSession* session) {}
	virtual void OnSessionDestroy(ObjCacheType_t& out_cache, TickSession* session, SessionStatus destroyReason) {}

};