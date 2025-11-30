#include "HeartbeatManager.hpp"
#include "Net.hpp"



HeartbeatManager::HeartbeatManager() : SessionManager(HEARTBEAT_MANAGER)
{

}

bool HeartbeatManager::Update(ObjCacheType_t& out_cache, double dt)
{
    bool ret = false;

    return ret;
}

bool HeartbeatManager::Receive(EasyServer* server, ObjCacheType_t& in_cache, ObjCacheType_t& out_cache)
{
    bool ret = false;

    for (auto& [session, cache] : in_cache)
    {
        for (EasySerializeable* obj : cache)
        {
            if (sHearbeat* heartbeat = dynamic_cast<sHearbeat*>(obj); heartbeat)
            {
                out_cache[session].push_back(new sHearbeat());
                std::cout << "[HeartbeatMng] Heartbeat received and sent!\n";
            }
            else if (sLogoutRequest* logoutRequest = dynamic_cast<sLogoutRequest*>(obj); logoutRequest)
            {
                out_cache[session].push_back(new sLogoutRequest());
                std::cout << "[HeartbeatMng] Logout request received!\n";
                session->logoutRequested = true;
            }
            else
            {
                //STATS_UNPROCESSED;
            }
        }
    }

    return ret;
}

void HeartbeatManager::OnSessionCreate(Session* session)
{
    
}

void HeartbeatManager::OnSessionDestroy(Session* session, SessionStatus destroyReason)
{

}