#include <EasyDB.hpp>
#include "Server.hpp"
#include "World.hpp"



Server::Server(EasyBufferManager* bf, unsigned short port) : EasyServer(bf, port), world(nullptr)
{
	
}

Server::~Server() 
{
    EasyServer::~EasyServer();
}

void Server::DoProcess(ObjCacheType_t& in_cache, ObjCacheType_t& out_cache)
{
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

    // Iterate receive cache
    {
        using BaseManagerCache = ObjCacheType_t;
        using ManagersCache = std::unordered_map<unsigned int, BaseManagerCache>;
        ManagersCache managersCache{};
        for (auto& [uid, pMng] : Server::managers)
            managersCache[uid];
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
            managers[manager.first]->Receive(this, manager.second, out_cache);
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

                    if (!alive || session->logoutRequested)
                    {
                        this->OnSessionDestroy(session, !alive ? TIMED_OUT : CLIENT_LOGGED_OUT);

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
    m->sessionsMutex.lock();
    for (auto& s : m->sessionIDs)
    {
        sDisconnectResponse acceptResponse = sDisconnectResponse("Server shutting down!");
        SendInstantPacket(m->sessions[s], { &acceptResponse });
    }
    m->sessionsMutex.unlock();

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
    
    if (status)
    {
        session->userData = new UD(session);

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
