#include "HeartbeatManager.hpp"



HeartbeatManager::HeartbeatManager(Server* server) : SessionManager(server, HEARTBEAT_MANAGER)
{

}

bool HeartbeatManager::Update(ObjCacheType_t& in_cache, ObjCacheType_t& out_cache, double dt)
{
    bool ret = false;

    return ret;
}

bool HeartbeatManager::Receive(ObjCacheType_t& in_cache, ObjCacheType_t& out_cache)
{
    bool ret = false;

    for (auto& [sid, cache] : in_cache)
    {
        TickSession* session = server->sessions[sid];
        for (auto it = cache.begin(); it != cache.end(); )
        {
            if (sHearbeat* heartbeat = dynamic_cast<sHearbeat*>(*it); heartbeat)
            {
                out_cache[sid].push_back(new sHearbeat());
                std::cout << "[HeartbeatMng] Heartbeat received and sent!\n";
                delete* it;
                it = cache.erase(it);
                ret = true;
            }
            else if (sLogoutRequest* logoutRequest = dynamic_cast<sLogoutRequest*>(*it); logoutRequest)
            {
                std::cout << "[HeartbeatMng] Logout request received!\n";
                session->logoutRequested = true;
                delete* it;
                it = cache.erase(it);
                ret = true;
            }
            else
            {
                //STATS_UNPROCESSED;
                it++;
            }
        }
    }

    return ret;
}

void HeartbeatManager::OnSessionCreate(ObjCacheType_t& out_cache, TickSession* session)
{
    
}

void HeartbeatManager::OnSessionDestroy(ObjCacheType_t& out_cache, TickSession* session, SessionStatus destroyReason)
{

}