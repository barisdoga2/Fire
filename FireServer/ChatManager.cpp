#include "ChatManager.hpp"

ObjCacheType_t messageCache;

ChatManager::ChatManager(Server* server) : SessionManager(server, CHAT_MANAGER)
{

}

bool ChatManager::Update(ObjCacheType_t& out_cache, double dt)
{
    bool ret = false;

    return ret;
}

bool ChatManager::Receive(ObjCacheType_t& in_cache, ObjCacheType_t& out_cache)
{
    bool ret = false;

    std::vector<sChatMessage*> messages;
    for (auto& [session, cache] : in_cache)
    {
        for (auto it = cache.begin(); it != cache.end(); )
        {
            if (sChatMessage* chatMessage = dynamic_cast<sChatMessage*>(*it); chatMessage)
            {
                messages.push_back(new sChatMessage(chatMessage->message, server->sessions[session]->username, Clock::now().time_since_epoch().count()));
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

    return ret;
}

void ChatManager::OnSessionCreate(TickSession* session)
{
    unsigned long long ts = Clock::now().time_since_epoch().count();
    for (auto& [sid, ses] : server->sessions)
        messageCache[sid].push_back(new sChatMessage(session->username + " has joined the game!", "[Server]", ts));
}

void ChatManager::OnSessionDestroy(TickSession* session, SessionStatus destroyReason)
{
    unsigned long long ts = Clock::now().time_since_epoch().count();
    for (auto& [sid, ses] : server->sessions)
        messageCache[sid].push_back(new sChatMessage(session->username + " has left the game!", "[Server]", ts));
}