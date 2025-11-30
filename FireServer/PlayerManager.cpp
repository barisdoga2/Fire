#include "PlayerManager.hpp"

PlayerManager::PlayerManager(Server* server) : SessionManager(server, PLAYER_MANAGER)
{

}

bool PlayerManager::Update(ObjCacheType_t& in_cache, ObjCacheType_t& out_cache, double dt)
{
    bool ret = false;

    static Timestamp_t nextUpdate = Clock::now();
    Timestamp_t currentTime = Clock::now();
    if (nextUpdate < currentTime)
    {
        nextUpdate = currentTime + Millis_t(1000U);

        for (auto& [sid, cache] : in_cache)
        {
            for (auto it = cache.begin(); it != cache.end(); )
            {
                if (sPlayerMovement* playerMovement = dynamic_cast<sPlayerMovement*>(*it); playerMovement)
                {
                    std::cout << "[PlayerMng] Player movement message received!\n";
                    if (server->sessions[sid]->userID == playerMovement->userID)
                    {
                        server->sessions[sid]->position = playerMovement->position;
                        server->sessions[sid]->rotation = playerMovement->rotation;
                        server->sessions[sid]->direction = playerMovement->direction;
                        server->sessions[sid]->moveTimestamp = playerMovement->timestamp;
                    }

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
        std::vector<sPlayerMovement> movements;
        for (auto& [sid, session] : server->sessions)
            movements.emplace_back(session->userID, session->position, session->rotation, session->direction, session->moveTimestamp);
        if (movements.size() > 0U && server->sessions.size() > 0U)
        {
            std::cout << "[PlayerMng] Player movement pack sent!\n";

            for (auto& [sid, session] : server->sessions)
                out_cache[sid].push_back(new sPlayerMovementPack(movements));
        }
    }

    return ret;
}

bool PlayerManager::Receive(ObjCacheType_t& in_cache, ObjCacheType_t& out_cache)
{
    bool ret = false;

    return ret;
}

void PlayerManager::OnSessionCreate(ObjCacheType_t& out_cache, TickSession* session)
{
    
}

void PlayerManager::OnSessionDestroy(ObjCacheType_t& out_cache, TickSession* session, SessionStatus destroyReason)
{
    
}