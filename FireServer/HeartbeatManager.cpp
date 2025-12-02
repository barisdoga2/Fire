#include "HeartbeatManager.hpp"



HeartbeatManager::HeartbeatManager(Server* server) : SessionManager(server, HEARTBEAT_MANAGER)
{

}

bool HeartbeatManager::Update(ObjCacheType_t& in_cache, ObjCacheType_t& out_cache, double dt)
{
    bool ret = false;

    static Timestamp_t nextUpdate = Clock::now();
    Timestamp_t currentTime = Clock::now();
    if (nextUpdate < currentTime)
    {
        nextUpdate = currentTime + Millis_t(1000U);

        for (auto& [sid, cache] : in_cache)
        {
            auto r = server->sessions.find(sid);
            if (r == server->sessions.end())
            {
                for (auto it = cache.begin(); it != cache.end(); )
                {
                    delete* it;
                    it++;
                }
                cache.clear();
                continue;
            }
            TickSession* session = r->second;

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
    }

    return ret;
}

bool HeartbeatManager::Receive(ObjCacheType_t& in_cache, ObjCacheType_t& out_cache)
{
    bool ret = false;

    return ret;
}

void HeartbeatManager::OnSessionCreate(ObjCacheType_t& out_cache, TickSession* session)
{
    
}

void HeartbeatManager::OnSessionDestroy(ObjCacheType_t& out_cache, TickSession* session, SessionStatus destroyReason)
{

}