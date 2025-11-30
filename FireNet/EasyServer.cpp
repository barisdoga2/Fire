#include "pch.h"
#include "EasyServer.hpp"

#include "EasySocket.hpp"
#include "EasyBuffer.hpp"
#include "EasyPacket.hpp"
#include "EasyDB.hpp"
#include "EasySerializer.hpp"

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

    this->OnDestroy();

    EasyServer::instance = nullptr;

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

void EasyServer::Broadcast(const std::vector<EasySerializeable*>& cache)
{
    m->sessionsMutex.lock();
    for (auto& sid : m->sessionIDs)
    {
        Session* ses = m->sessions[sid];
        SendInstantPacket(ses, cache);
    }
    m->sessionsMutex.unlock();
}

bool EasyServer::CreateSession_internal(Session* session)
{
    bool ret = false;
    if(m->sessions[session->sessionID] == nullptr)
    {
        m->sessions[session->sessionID] = session;
        ret = this->OnSessionCreate(session);
        if (ret)
        {
            m->sessionIDs.push_back(session->sessionID);
        }
    }
    return ret;
}

bool EasyServer::DestroySession_external(SessionID_t sessionID, SessionStatus disconnectReason)
{
    bool ret = false;
    if (m->sessions[sessionID] != nullptr)
    {
        this->OnSessionDestroy(m->sessions[sessionID], disconnectReason);
        m->sessionIDs.erase(std::find(m->sessionIDs.begin(), m->sessionIDs.end(), sessionID));
        delete m->sessions[sessionID];
        m->sessions[sessionID] = nullptr;
    }
    ret = true;
    return ret;
}

bool EasyServer::DestroySession_internal(Session* session, SessionStatus disconnectReason)
{
    bool ret = false;
    SessionID_t sid = session->sessionID;
    if (m->sessions[sid] != nullptr)
    {
        this->OnSessionDestroy(m->sessions[sid], disconnectReason);
        if(auto res = std::find(m->sessionIDs.begin(), m->sessionIDs.end(), sid); res != m->sessionIDs.end())
            m->sessionIDs.erase(res);
        delete m->sessions[sid];
        m->sessions[sid] = nullptr;
        ret = true;
    }
    return ret;
}

bool EasyServer::SendInstantPacket(Session* destination, const std::vector<EasySerializeable*>& objs)
{
     EasyBuffer* serializationBuffer = bf->Get();
     EasyBuffer* sendBuffer = bf->Get();

    bool status = (objs.size() > 0) && (destination) && (sendBuffer) && (serializationBuffer);
    if (status)
    {
        if (MakeSerialized(serializationBuffer, objs))
        {
            if (EasyPacket::MakeCompressed(serializationBuffer, sendBuffer))
            {
                PeerCryptInfo crypt(destination->sessionID, destination->sequenceID_in, destination->sequenceID_out, destination->key);
                if (EasyPacket::MakeEncrypted(crypt, sendBuffer))
                {
                    //STATS_ENCRYPTED;

                    uint64_t res = sock->send(sendBuffer->begin(), sendBuffer->m_payload_size + EasyPacket::HeaderSize(), destination->addr);
                    if (res != WSAEISCONN)
                    {
                        //STATS_SEND_FAIL;
                        status = false;
                    }
                    else
                    {
                        //STATS_SEND;
                        status = true;
                    }
                }
                else
                {
                    //STATS_ENCRYPT_FAIL;
                    status = false;
                }
            }
            else
            {
                //STATS_COMPRESS_FAIL;
                status = false;
            }
        }
        else
        {
            //STATS_SERIALIZE_FAIL;
            status = false;
        }
        ++destination->sequenceID_out;
    }
    else if (!sendBuffer || !serializationBuffer)
    {
        if(!sendBuffer)
            sendBuffer = bf->Get();
        if (!serializationBuffer)
            serializationBuffer = bf->Get();
    }
    bf->Free(serializationBuffer);
    bf->Free(sendBuffer);
    return status;
}