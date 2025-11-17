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
                if (true/*ECHO*/)
                {
                    static int i = 0;
                    pHello* echo_hello = new pHello("Hi! I'm server." + std::to_string(++i) + ". You said: " + hello->message);
                    if (auto res = out_cache.find(it->first); res != out_cache.end())
                    {
                        res->second.push_back(echo_hello);
                    }
                    else
                    {
                        out_cache.emplace(it->first, std::vector<EasySerializeable*>{ echo_hello });
                    }
                }
                //STATS_PROCESSED;
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

void Server::OnSessionCreate(Session* session)
{

}

void Server::OnSessionDestroy(Session* session)
{

}
