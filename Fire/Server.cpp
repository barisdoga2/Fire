#include "Server.hpp"
#include "World.hpp"



Server::Server(EasyBufferManager* bf, unsigned short port) : EasyServer(bf, 54000), world(nullptr)
{
	
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