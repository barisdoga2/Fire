#include "ChatManager.hpp"

ObjCacheType_t messageCache;

ChatManager::ChatManager(FireServer* server) : SessionManager3(server, CHAT_MANAGER)
{

}

bool ChatManager::Update(ObjCacheType_t& in_cache, ObjCacheType_t& out_cache, double dt)
{
    bool ret = false;

    static Timestamp_t nextUpdate = Clock::now();
    Timestamp_t currentTime = Clock::now();
    if (nextUpdate < currentTime)
    {
        nextUpdate = currentTime + Millis_t(1U);

        std::vector<sChatMessage*> messages;
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
            FireSession* session = r->second;

            for (auto it = cache.begin(); it != cache.end(); )
            {
                if (sChatMessage* chatMessage = dynamic_cast<sChatMessage*>(*it); chatMessage)
                {
                    messages.push_back(new sChatMessage(chatMessage->message, session->username, Clock::now().time_since_epoch().count()));
                    std::cout << "[ChatMng] Chat message received!\n";
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

        if (messageCache.size() > 0U)
        {
            for (const auto& [sid, cache] : messageCache)
            {
                auto& dstVec = out_cache[sid];
                dstVec.insert(dstVec.end(), cache.begin(), cache.end());
            }
            messageCache.clear();
        }

        if (ret)
            for (auto& [session, cache] : server->sessions)
                out_cache[session].insert(out_cache[session].end(), messages.begin(), messages.end());
    }

    return ret;
}

bool ChatManager::Receive(ObjCacheType_t& in_cache, ObjCacheType_t& out_cache)
{
    bool ret = false;

    return ret;
}

void ChatManager::OnSessionCreate(ObjCacheType_t& out_cache, FireSession* session)
{
    unsigned long long ts = Clock::now().time_since_epoch().count();
    for (auto& [sid, ses] : server->sessions)
        messageCache[sid].push_back(new sChatMessage(session->username + " has joined the game!", "[Server]", ts));
}

void ChatManager::OnSessionDestroy(ObjCacheType_t& out_cache, FireSession* session, SessionStatus destroyReason)
{
    unsigned long long ts = Clock::now().time_since_epoch().count();
    for (auto& [sid, ses] : server->sessions)
        messageCache[sid].push_back(new sChatMessage(session->username + " has left the game!", "[Server]", ts));
}