#include <EasyDB.hpp>

#include "LoginManager.hpp"
#include "Net.hpp"
#include "Server.hpp"



LoginManager::LoginManager() : SessionManager(LOGIN_MANAGER)
{

}

bool LoginManager::Update(ObjCacheType_t& out_cache, double dt)
{
    bool ret = false;

    return ret;
}

bool LoginManager::Receive(EasyServer* server, ObjCacheType_t& in_cache, ObjCacheType_t& out_cache)
{
    bool ret = false;

    for (auto& [session, cache] : in_cache)
    {
        UserStats* stats = ((UD*)session->userData)->stats;
        for (EasySerializeable* obj : cache)
        {
            if (sChampionSelectRequest* championSelectRequest = dynamic_cast<sChampionSelectRequest*>(obj); championSelectRequest || !stats)
            {
                //STATS_PROCESSED;
                if (stats)
                {
                    bool response = std::find(stats->champions_owned.begin(), stats->champions_owned.end(), championSelectRequest->championID) != stats->champions_owned.end();
                    std::string message{};
                    if (!response)
                        message = "Buy this champion first!";
                    else
                    {
                        for (std::vector<SessionManager*>::iterator manIt = session->managers.begin(); manIt != session->managers.end(); )
                        {
                            if ((*manIt)->managerID == LOGIN_MANAGER)
                            {
                                manIt = session->managers.erase(manIt);
                            }
                            else
                            {
                                manIt++;
                            }
                        }
                        session->managers.push_back(Server::managers[HEARTBEAT_MANAGER]);
                    }
                    out_cache[session].push_back(new sChampionSelectResponse(response, message));
                    std::cout << "[LoginMng] Champion select request received\n";
                }
                else
                {
                    sql::PreparedStatement* stmt2 = server->db->PrepareStatement("SELECT * FROM user_stats WHERE user_id=? LIMIT 1;");
                    stmt2->setUInt(1, session->userID);
                    sql::ResultSet* res2 = stmt2->executeQuery();
                    if (res2->next())
                    {
                        //STATS_PROCESSED;
                        ((UD*)session->userData)->stats = new UserStats();
                        stats = ((UD*)session->userData)->stats;

                        stats->userID = res2->getUInt(2);
                        stats->gametime = res2->getUInt(3);
                        stats->golds = res2->getUInt(4);
                        stats->diamonds = res2->getUInt(5);
                        stats->tutorial_done = res2->getBoolean(6);
                        std::string champions = res2->getString(7);
                        while (champions.length() != 0U)
                        {
                            if (auto pos = champions.find(", "); pos != std::string::npos)
                            {
                                std::string champion = champions.substr(0, pos);
                                stats->champions_owned.push_back(static_cast<unsigned int>(std::stoul(champion)));
                                champions = champions.substr(pos + 2);
                            }
                            else
                            {
                                stats->champions_owned.push_back(static_cast<unsigned int>(std::stoul(champions)));
                                champions = "";
                            }
                        }

                        out_cache[session].push_back(new sPlayerBootInfo(stats->userID, stats->diamonds, stats->golds, stats->gametime, stats->tutorial_done, stats->champions_owned));

                    }
                    else
                    {
                        //STATS_UNPROCESSED;
                    }
                    delete res2;
                    delete stmt2;
                }
            }
            else if (sLogoutRequest* logoutRequest = dynamic_cast<sLogoutRequest*>(obj); logoutRequest)
            {
                out_cache[session].push_back(new sLogoutRequest());
                std::cout << "[LoginMng] Logout request received!\n";
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
