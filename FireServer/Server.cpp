#include <EasyDB.hpp>
#include "Server.hpp"
#include "World.hpp"


#include "HeartbeatManager.hpp"
#include "LoginManager.hpp"
#include "ChatManager.hpp"

std::unordered_map<SessionManagers, SessionManager*> all_managers{};

TickSession::TickSession(Session* session) : sessionID(session->sessionID), userID(session->userID), addr(session->addr), lastReceive(session->lastReceive), logoutRequested(false)
{
    
}

TickSession::~TickSession()
{
    for (auto& [mid, manager] : this->managers)
        manager->OnSessionDestroy(this, status);
}

void TickSession::RegisterToManager(SessionManagers sessionManager)
{
    this->managers.emplace(sessionManager, all_managers[sessionManager]);
    all_managers[sessionManager]->OnSessionCreate(this);
}

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
        sessionsMutex.lock();
        for (auto& [sid, session] : in_cache)
        {
            sessions[sid]->lastReceive = Clock::now();
        }
        for (auto& [mid, manager] : all_managers)
        {
            manager->Receive(in_cache, out_cache);
        }

        in_cache.clear();
        sessionsMutex.unlock();
    }

    // Iterate for timeouts
    {
        static TimePoint nextTimeoutCheck = Clock::now() + std::chrono::seconds(1);
        const TimePoint now = Clock::now();

        static std::vector<TickSession*> destroyCache{};
        if (now >= nextTimeoutCheck)
        {
            if (sessionsMutex.try_lock())
            {
                nextTimeoutCheck = now + std::chrono::seconds(1);
                for (auto& [sid, session] : sessions)
                {
                    const bool timeout = std::chrono::duration_cast<std::chrono::milliseconds>(now - session->lastReceive) < std::chrono::milliseconds(TickSession::sessionTimeout);
                    if (!timeout || session->logoutRequested)
                    {
                        if (std::find(destroyCache.begin(), destroyCache.end(), session) == destroyCache.end())
                            destroyCache.push_back(session);
                    }
                }

                sessionsMutex.unlock();
            }
            if (destroyCache.size() > 0U)
            {
                if (m->sessionsMutex.try_lock())
                {
                    for (auto it = destroyCache.begin(); it != destroyCache.end(); )
                    {
                        if (DestroySession_external((*it)->sessionID, (*it)->logoutRequested ? CLIENT_LOGGED_OUT : TIMED_OUT))
                        {
                            it = destroyCache.erase(it);
                        }
                        else
                        {
                            it++;
                        }
                    }
                    m->sessionsMutex.unlock();
                }
            }
        }
    }
}

bool Server::Start()
{
    bool ret = EasyServer::Start();
    if (ret)
    {
        all_managers[LOGIN_MANAGER] = new LoginManager(this);
        all_managers[HEARTBEAT_MANAGER] = new HeartbeatManager(this);
        all_managers[CHAT_MANAGER] = new ChatManager(this);
    }
    return ret;
}

void Server::Tick(double _dt)
{

}

void Server::OnDestroy()
{
    sessionsMutex.lock();
    for (auto& [sid, session] : sessions)
        Broadcast({ new sDisconnectResponse("Server shutting down!") });
    sessionsMutex.unlock();
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
    sessionsMutex.lock();

    bool status = true;
    
    if (status)
    {
        TickSession* tSession = new TickSession(session);
        tSession->RegisterToManager(LOGIN_MANAGER);
        sessions[session->sessionID] = tSession;
        
        sLoginResponse acceptResponse = sLoginResponse(true, "Server welcomes you!");
        SendInstantPacket(session, { &acceptResponse });

    }

    sessionsMutex.unlock();

    return status;
}

void Server::OnSessionDestroy(Session* session, SessionStatus disconnectReason)
{
    sessionsMutex.lock();

    sDisconnectResponse disconnectResponse = sDisconnectResponse("Disconnect reason: '" + SessionStatus_Str(disconnectReason) + "'!");
    SendInstantPacket(session, { &disconnectResponse });

    auto res = sessions.find(session->sessionID);
    delete res->second;
    sessions.erase(res);

    sessionsMutex.unlock();
}
