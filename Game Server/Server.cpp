#include "Server.hpp"
#include "World.hpp"



Server::Server(EasyBufferManager* bf, unsigned short port) : EasyServer(bf, port), world(nullptr)
{
	
}

void Server::DoProcess(ObjCacheType_t& in_cache, ObjCacheType_t& out_cache)
{
    for (ObjCacheType_t::iterator it = in_cache.begin(); it != in_cache.end(); ++it)
    {
        for (EasySerializeable* in_obj : it->second)
        {
            if (pHello* hello = dynamic_cast<pHello*>(in_obj); hello)
            {
                //STATS_PROCESSED;
                //if (true/*ECHO*/)
                //{
                //    static int i = 0;
                //    pHello* echo_hello = new pHello("Hi! I'm server." + std::to_string(++i) + ". You said: " + hello->message);
                //    if (auto res = out_cache.find(it->first); res != out_cache.end())
                //    {
                //        res->second.push_back(echo_hello);
                //    }
                //    else
                //    {
                //        out_cache.emplace(it->first, std::vector<EasySerializeable*>{ echo_hello });
                //    }
                //}
                std::cout << "Hello message received: '" << hello->message << "\n";
            }
            else
            {
                //STATS_UNPROCESSED;
            }
            
            if (sChampionSelectRequest* championSelectRequest = dynamic_cast<sChampionSelectRequest*>(in_obj); championSelectRequest)
            {
                //STATS_PROCESSED;
                
                if (Session* session = GetSession(it->first); session)
                {
                    bool response = std::find(session->stats.champions_owned.begin(), session->stats.champions_owned.end(), championSelectRequest->championID) != session->stats.champions_owned.end();
                    std::string message{};
                    if (!response)
                        message = "Buy this champion first!";
                    sChampionSelectResponse* championSelectResponse = new sChampionSelectResponse(response, message);
                    if (auto res = out_cache.find(it->first); res != out_cache.end())
                    {
                        res->second.push_back(championSelectResponse);
                    }
                    else
                    {
                        out_cache.emplace(it->first, std::vector<EasySerializeable*>{ championSelectResponse });
                    }
                }
                
                std::cout << "Champion select request received\n";
            }
            else
            {
                //STATS_UNPROCESSED;
            }

            if (sHearbeat* heartbeat = dynamic_cast<sHearbeat*>(in_obj); heartbeat)
            {
                //STATS_PROCESSED;

                std::cout << "Heartbeat received\n";
            }
            else
            {
                //STATS_UNPROCESSED;
            }
            delete in_obj;
        }
    }
    in_cache.clear();

    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

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
    static int i = 0;
    i++;
    if (i % 3 == 0)
    {
        sLoginResponse rejectResponse = sLoginResponse(false, "Server just wanted to reject.");
        SendInstantPacket(session, { &rejectResponse });
        return false;
    }
    else
    {
        sLoginResponse acceptResponse = sLoginResponse(true, "Server welcomes you!");
        SendInstantPacket(session, { &acceptResponse });

        sPlayerBootInfo playerBootInfo = sPlayerBootInfo(session->stats.userID, session->stats.diamonds, session->stats.golds, session->stats.gametime, session->stats.tutorial_done, session->stats.champions_owned);
        SendInstantPacket(session, { &playerBootInfo });

        return true;
    }
}

void Server::OnSessionDestroy(Session* session, SessionStatus disconnectReason)
{
    sDisconnectResponse disconnectResponse = sDisconnectResponse("Disconnect reason: '" + SessionStatus_Str(disconnectReason) + "'!");
    SendInstantPacket(session, { &disconnectResponse });
}
