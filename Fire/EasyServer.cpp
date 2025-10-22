#include "EasyServer.hpp"

#include "EasySocket.hpp"
#include "EasyBuffer.hpp"
#include "EasyPacket.hpp"
#include "EasyDB.hpp"
#include "World.hpp"
#include "Serializer.hpp"

#include <unordered_map>
#include <array>



EasyServer::EasyServer(EasyBufferManager* bf, unsigned short port) : bf(bf), running(false), port(port), sock(nullptr), db(nullptr), m(nullptr)
{

}

EasyServer::~EasyServer()
{
    Stop();
}

bool EasyServer::Start()
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

    running = true;

    EasyServer::instance = this;

    this->OnInit();

    m = new MainContex();

    receive = std::thread(&EasyServer::Receive, this);
    update = std::thread(&EasyServer::Update, this);
    send = std::thread(&EasyServer::Send, this);

    return true;
}

bool EasyServer::Stop()
{
    if (!running)
        return false;

    std::cout << "Server is stopping..." << std::endl;

    running = false;

    EasyServer::instance = nullptr;

    this->OnDestroy();

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

int EasyServer::Stats(lua_State* L)
{
    std::cout << EasyServer::instance->StatsReceive() << EasyServer::instance->StatsSend() << EasyServer::instance->StatsUpdate();
    return 0;
}

bool EasyServer::IsRunning()
{
    return running;
}

bool EasyServer::CreateSession(Session* session)
{
    bool ret = false;
    if(m->sessions[session->sessionID] == nullptr)
    {
        m->sessions[session->sessionID] = session;
        this->OnSessionCreate(session);
        ret = true;
    }
    return ret;
}