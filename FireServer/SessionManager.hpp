#pragma once

#include "FireServer.hpp"

class FireSession;
class SessionManager3 {
public:
	FireServer* server;
	unsigned int managerID;

	SessionManager3(FireServer* server, unsigned int managerID) : server(server), managerID(managerID)
	{

	}

	virtual bool Update(ObjCacheType_t& in_cache, ObjCacheType_t& out_cache, double dt) = 0U;
	virtual bool Receive(ObjCacheType_t& in_cache, ObjCacheType_t& out_cache) = 0U;
	virtual void OnSessionCreate(ObjCacheType_t& out_cache, FireSession* session) {}
	virtual void OnSessionDestroy(ObjCacheType_t& out_cache, FireSession* session, SessionStatus destroyReason) {}

};