#include "Server.hpp"
#include "World.hpp"
#include <EasyDB.hpp>


bool HeartbeatManager::Update(ObjCacheType_t& out_cache, double dt)
{
    bool ret = false;

    return ret;
}

bool HeartbeatManager::Receive(EasyDB* db, ObjCacheType_t& in_cache, ObjCacheType_t& out_cache)
{
    bool ret = false;

    return ret;
}

bool LoginManager::Update(ObjCacheType_t& out_cache, double dt)
{
    bool ret = false;

    return ret;
}

bool LoginManager::Receive(EasyDB* db, ObjCacheType_t& in_cache, ObjCacheType_t& out_cache)
{
    bool ret = false;

    for (auto& [session, cache] : in_cache)
    {
        UserStats* stats = ((UD*)session->userData)->stats;
        if (stats)
        {
            for (EasySerializeable* obj : cache)
            {
                if (sChampionSelectRequest* championSelectRequest = dynamic_cast<sChampionSelectRequest*>(obj); championSelectRequest)
                {
                    //STATS_PROCESSED;
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
                    }
                    out_cache[session].push_back(new sChampionSelectResponse(response, message));
                    std::cout << "Champion select request received\n";
                }
                else
                {
                    //STATS_UNPROCESSED;
                }
            }
        }
        else
        {
            sql::PreparedStatement* stmt2 = db->PrepareStatement("SELECT * FROM user_stats WHERE user_id=? LIMIT 1;");
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

    return ret;
}


Server::Server(EasyBufferManager* bf, unsigned short port) : EasyServer(bf, port), world(nullptr)
{
	
}

void Server::DoProcess(ObjCacheType_t& in_cache, ObjCacheType_t& out_cache)
{
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

    // Iterate receive cache
    {
        using BaseManagerCache = ObjCacheType_t;
        using ManagersCache = std::unordered_map<unsigned int, BaseManagerCache>;

        ManagersCache managersCache = { {1U, {}}, {2U, {}}};
        for (auto& [session, cache] : in_cache)
        {
            for (SessionManager* manager : session->managers)
            {
                managersCache[manager->managerID][session].insert(managersCache[manager->managerID][session].end(), cache.begin(), cache.end());
            }
        }
        in_cache.clear();

        for (auto& manager : managersCache)
        {
            managers[manager.first]->Receive(db, manager.second, out_cache);
        }
    }

    // Iterate for timeouts
    {
        static TimePoint nextTimeoutCheck = Clock::now() + std::chrono::seconds(1);
        const TimePoint now = Clock::now();
        if (now >= nextTimeoutCheck)
        {
            nextTimeoutCheck = now + std::chrono::seconds(1);

            for (auto it = m->sessionIDs.begin(); it != m->sessionIDs.end(); )
            {
                Session* session = GetSession(*it);
                if (session)
                {
                    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                        now - session->lastReceive
                    );

                    const bool alive = elapsed < std::chrono::milliseconds(sessionTimeout);

                    if (!alive)
                    {
                        this->OnSessionDestroy(session, TIMED_OUT);

                        delete m->sessions[*it];
                        m->sessions[*it] = nullptr;

                        it = m->sessionIDs.erase(it);
                    }
                    else
                    {
                        ++it;
                    }
                }
                else
                {
                    ++it;
                }
            }
        }
    }
}

bool Server::Start()
{
	return EasyServer::Start();
}

void Server::Tick(double _dt)
{

}

void Server::OnDestroy()
{
	if (world)
	{
		delete world;
		world = nullptr;
	}
}

void Server::OnInit()
{
	if (!world)
	{
		world = new World();
	}
}

bool Server::OnSessionCreate(Session* session)
{
    bool status = true;

    session->userData = new UD(session);

    status = true;
    if (status)
    {
        session->managers.push_back(managers[LOGIN_MANAGER]);
        sLoginResponse acceptResponse = sLoginResponse(true, "Server welcomes you!");
        SendInstantPacket(session, { &acceptResponse });
    }

    return status;
}

void Server::OnSessionDestroy(Session* session, SessionStatus disconnectReason)
{
    sDisconnectResponse disconnectResponse = sDisconnectResponse("Disconnect reason: '" + SessionStatus_Str(disconnectReason) + "'!");
    SendInstantPacket(session, { &disconnectResponse });
}
