#include <EasyDB.hpp>
#include "LoginManager.hpp"



LoginManager::LoginManager(Server* server) : SessionManager(server, LOGIN_MANAGER)
{

}

bool LoginManager::Update(ObjCacheType_t& out_cache, double dt)
{
    bool ret = false;

    return ret;
}

bool LoginManager::Receive(ObjCacheType_t& in_cache, ObjCacheType_t& out_cache)
{
    bool ret = false;

    for (auto& [sid, cache] : in_cache)
    {
        TickSession* session = server->sessions[sid];
        if (session->stats.userID == 0U)
        {
            sql::PreparedStatement* stmt = server->db->PrepareStatement("SELECT * FROM users WHERE id=? LIMIT 1;");
            stmt->setUInt(1, session->userID);
            sql::ResultSet* res = stmt->executeQuery();
            if (res->next())
            {
                session->username = std::string(res->getString(2).c_str());
            }
            else
            {
                //STATS_UNPROCESSED;
            }
            delete res;
            delete stmt;

            sql::PreparedStatement* stmt2 = server->db->PrepareStatement("SELECT * FROM user_stats WHERE user_id=? LIMIT 1;");
            stmt2->setUInt(1, session->userID);
            sql::ResultSet* res2 = stmt2->executeQuery();
            if (res2->next())
            {
                session->stats.userID = res2->getUInt(2);
                session->stats.gametime = res2->getUInt(3);
                session->stats.golds = res2->getUInt(4);
                session->stats.diamonds = res2->getUInt(5);
                session->stats.tutorial_done = res2->getBoolean(6);
                std::string champions = res2->getString(7);
                while (champions.length() != 0U)
                {
                    if (auto pos = champions.find(", "); pos != std::string::npos)
                    {
                        std::string champion = champions.substr(0, pos);
                        session->stats.champions_owned.push_back(static_cast<unsigned int>(std::stoul(champion)));
                        champions = champions.substr(pos + 2);
                    }
                    else
                    {
                        session->stats.champions_owned.push_back(static_cast<unsigned int>(std::stoul(champions)));
                        champions = "";
                    }
                }

                out_cache[sid].push_back(new sPlayerBootInfo(session->stats.userID, session->stats.diamonds, session->stats.golds, session->stats.gametime, session->stats.tutorial_done, session->stats.champions_owned));

            }
            else
            {
                //STATS_UNPROCESSED;
            }
            delete res2;
            delete stmt2;
        }
        else
        {
            for (auto it = cache.begin(); it != cache.end(); )
            {
                if (sChampionSelectRequest* championSelectRequest = dynamic_cast<sChampionSelectRequest*>(*it); championSelectRequest)
                {
                    bool response = std::find(session->stats.champions_owned.begin(), session->stats.champions_owned.end(), championSelectRequest->championID) != session->stats.champions_owned.end();
                    std::string message{};
                    if (!response)
                        message = "Buy this champion first!";
                    else
                    {
                        session->RegisterToManager(HEARTBEAT_MANAGER);
                        session->RegisterToManager(CHAT_MANAGER);
                    }
                    out_cache[sid].push_back(new sChampionSelectResponse(response, message));
                    std::cout << "[LoginMng] Champion select request received\n";
                    delete* it;
                    it = cache.erase(it);
                }
                else if (sLogoutRequest* logoutRequest = dynamic_cast<sLogoutRequest*>(*it); logoutRequest)
                {
                    out_cache[sid].push_back(new sLogoutRequest());
                    std::cout << "[LoginMng] Logout request received!\n";
                    session->logoutRequested = true;
                    delete* it;
                    it = cache.erase(it);
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
