#include "Server.hpp"

#include "EasySocket.hpp"
#include "EasyBuffer.hpp"
#include "EasyPacket.hpp"
#include "EasyDB.hpp"
#include "World.hpp"
#include "Serializer.hpp"

#include <unordered_map>
#include <array>



Server::Server(EasyBufferManager* bf, unsigned short port) : bf(bf), running(false), port(port), sock(nullptr), db(nullptr), m(nullptr), world(nullptr)
{

}

Server::~Server()
{
    Stop();
}

bool Server::Start()
{
    if (running)
        return false;

    std::cout << "Server is starting..." << std::endl;

    sock = new EasySocket();
    if (sock->bind(port) != WSAEISCONN)
    {
        std::cout << "  Couldn't bind the port." << std::endl;
        delete sock;
        sock = nullptr;
        return false;
    }

    db = new EasyDB();
    if (!db->IsConnected())
    {
        std::cout << "  Couldn't connect to the database." << std::endl;
        delete db;
        delete sock;
        sock = nullptr;
        db = nullptr;
        return false;
    }

    world = new World();

    m = new MainContex();

    running = true;

    Server::instance = this;

    receive = std::thread(&Server::Receive, this);
    update = std::thread(&Server::Update, this);
    send = std::thread(&Server::Send, this);

    return true;
}

bool Server::Stop()
{
    if (!running)
        return false;

    std::cout << "Server is stopping..." << std::endl;

    running = false;

    Server::instance = nullptr;

    delete world;
    world = nullptr;
    delete db;
    db = nullptr;
    delete m;
    m = nullptr;
    delete sock;
    sock = nullptr;

    if (receive.joinable())
        receive.join();
    if (update.joinable())
        update.join();
    if (send.joinable())
        send.join();

    return true;
}

int Server::Stats(lua_State* L)
{
    std::cout << Server::instance->StatsReceive() << Server::instance->StatsSend() << Server::instance->StatsUpdate();
    return 0;
}

bool Server::IsRunning()
{
    return running;
}

bool Server::CreateSession(Session* session)
{
    bool ret = false;
    if(m->sessions[session->sessionID] == nullptr)
    {
        m->sessions[session->sessionID] = session;
        ret = true;
    }
    return ret;
}