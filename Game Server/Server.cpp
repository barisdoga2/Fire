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
            delete in_obj;
        }
    }
    in_cache.clear();
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
        return true;
    }
}

void Server::OnSessionDestroy(Session* session, SessionStatus disconnectReason)
{
    sDisconnectResponse disconnectResponse = sDisconnectResponse("Disconnect reason: '" + SessionStatus_Str(disconnectReason) + "'!");
    SendInstantPacket(session, { &disconnectResponse });
}
