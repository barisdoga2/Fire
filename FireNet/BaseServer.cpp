#include "pch.h"

#include "BaseServer.hpp"

#include <iostream>
#include <vector>
#include <array>
#include <unordered_map>
#include <chrono>
#include <atomic>
#include <sstream>
#include <random>
#include <optional>
#include <filesystem>
#include <ctime>

#include "EasyIpAddress.hpp"
#include "EasySocket.hpp"
#include "EasyBuffer.hpp"
#include "EasySerializer.hpp"
#include "EasyPacket.hpp"
#include "EasyNet.hpp"

class Session {
public:
    SessionID_t sid;
    Addr_t addr;
    Key_t key;
    Key_Expt_t key_expiry;
    SequenceID_t seqid_in, seqid_out;
    Timestamp_t lastReceive;
    Session() : sid(0U), addr(0U), key(16U), key_expiry(), seqid_in(0U), seqid_out(0U), lastReceive()
    {
    }
    Session(const Session& session) : sid(session.sid), addr(session.addr), key(session.key), key_expiry(session.key_expiry), seqid_in(session.seqid_in), seqid_out(session.seqid_out), lastReceive(session.lastReceive)
    {
    }
    Session(const SessionID_t& sid, const Addr_t& addr, const Key_t& key, const Key_Expt_t& key_expiry, const SequenceID_t& seqid_in, const SequenceID_t& seqid_out, const Timestamp_t& lastReceive) : sid(sid), addr(addr), key(key), key_expiry(key_expiry), seqid_in(seqid_in), seqid_out(seqid_out), lastReceive(lastReceive)
    {
    }

    SessionBase GetBase()
    {
        return {&sid, &addr, &key, &key_expiry, &lastReceive };
    }
};

class ReceiveManager;
class SendManager;
class SessionManager;
class ServerContext {
public:
    const bool encryption, compression;
    EasySocket* sck;
    EasyBufferManager* bufferMan;
    SessionManager* sessionMan;
    ReceiveManager* receiveMan;
    SendManager* sendMan;
    ServerCallback* cbk;

    ServerCache_t receiveCache;
    ServerCache_t sendCache;

    ServerContext(EasyBufferManager* bm, ServerCallback* cbk, bool encryption, bool compression) : bufferMan(bm), cbk(cbk), encryption(encryption), compression(compression), sessionMan(), receiveMan(), sendMan(), sck()
    {

    }

    ~ServerContext()
    {

    }
};

class SessionManager {
private:
    std::vector<SessionID_t> sessionIDs;
    std::array<Session*, MAX_SESSIONS> sessions{ nullptr };

    ServerContext* cntx;

public:
    SessionManager(ServerContext* cntx) : cntx(cntx)
    {

    }

    ~SessionManager()
    {
        for (const SessionID_t& sid : sessionIDs)
        {
            delete sessions[sid];
            sessions[sid] = nullptr;
        }
        sessionIDs.clear();
    }

    void Update(double dt)
    {

    }

    Session* CreateSession(const SessionID_t& sid, const Addr_t& addr, const Timestamp_t& lastReceive, EasyBuffer* buffer)
    {
        if (sessions[sid])
            return nullptr;
        if (Session* session = new Session(sid, addr, {}, {}, 0U, 0U, lastReceive); cntx->cbk->OnSessionCreate(session->GetBase(), buffer))
        {
            sessions[sid] = session;
            sessionIDs.push_back(sid);
            return session;
        }
        else
        {
            delete session;
        }
        return nullptr;
    }

    void DestroySession(SessionID_t sid, std::string disconnectMessage = "")
    {
        if (!sessions[sid])
            return;
        cntx->cbk->OnSessionDestroy(sessions[sid]->GetBase(), disconnectMessage);
        sessionIDs.erase(std::find(sessionIDs.begin(), sessionIDs.end(), sid));
        delete sessions[sid];
        sessions[sid] = nullptr;
    }

    Session* GetSession(SessionID_t sid)
    {
        return sessions[sid];
    }

};

class ReceiveManager {
    class RawPacket {
    public:
        SessionID_t sid;
        Addr_t addr;
        EasyBuffer* buff;
        Timestamp_t recv;
        RawPacket(const SessionID_t sid, const Addr_t addr, EasyBuffer* buff, const Timestamp_t recv) : sid(sid), addr(addr), buff(buff), recv(recv)
        {

        }
    };

    ServerContext* cntx;
    EasyBuffer* receiveBuffer;
    EasyBuffer* decompressBuffer;

public:
    ReceiveManager(ServerContext* cntx) : cntx(cntx), receiveBuffer(), decompressBuffer()
    {
        while (!receiveBuffer)
            receiveBuffer = cntx->bufferMan->Get();
        while (!decompressBuffer)
            decompressBuffer = cntx->bufferMan->Get();
    }

    ~ReceiveManager()
    {
        if(receiveBuffer)
            cntx->bufferMan->Free(receiveBuffer);
        if (decompressBuffer)
            cntx->bufferMan->Free(decompressBuffer);
    }

    void Update(double dt)
    {
        if (RawPacket* rcv = ReceiveOne(); rcv)
        {
            Session* session = cntx->sessionMan->GetSession(rcv->sid);

            if (!session)
            {
                session = cntx->sessionMan->CreateSession(rcv->sid, rcv->addr, rcv->recv, rcv->buff);
            }
            else
            {
                if (session->addr != rcv->addr)
                {
                    session = cntx->cbk->OnSessionReconnect(session->GetBase(), { &rcv->sid, &rcv->addr, &session->key, &session->key_expiry, &rcv->recv }) ? session : nullptr;
                }
                else
                {

                }
                if (session)
                    ProcessReceived(session, rcv);
                else
                    cntx->bufferMan->Free(rcv->buff);
            }
        }
    }

private:
    ReceiveManager::RawPacket* ReceiveOne()
    {
        if (!receiveBuffer)
            receiveBuffer = cntx->bufferMan->Get();
        if (!receiveBuffer)
            return nullptr;

        uint64_t addr;
        receiveBuffer->reset();
        if (uint64_t ret = cntx->sck->receive(receiveBuffer->begin(), receiveBuffer->capacity(), receiveBuffer->m_payload_size, addr); ret == WSAEISCONN)
        {
            if (receiveBuffer->m_payload_size >= EasyPacket::MinimumSize())
            {
                EasyPacket packet(receiveBuffer);
                SessionID_t sid = *packet.SessionID();
                if (IS_SESSION(sid))
                {
                    ReceiveManager::RawPacket* ret = new ReceiveManager::RawPacket(sid, addr, receiveBuffer, Clock::now());
                    receiveBuffer = nullptr;
                    return ret;
                }
            }
        }
        return nullptr;
    }

    bool ProcessReceived(Session* session, RawPacket* rcv)
    {
        bool ret = false;
        EasyPacket pck(rcv->buff);
        CryptData crypt(session->sid, session->seqid_in, session->seqid_out, session->key);
        if (pck.MakeDecrypted(crypt, rcv->buff))
        {
            decompressBuffer->reset();
            if (pck.MakeDecompressed(rcv->buff, decompressBuffer))
            {
                std::vector<EasySerializeable*> cache;
                if (MakeDeserialized(decompressBuffer, cache))
                {
                    cntx->receiveCache[session->sid].insert(cntx->receiveCache[session->sid].end(), cache.begin(), cache.end());
                    session->seqid_in++;
                    session->lastReceive = rcv->recv;
                    ret = true;
                }
            }
        }
        cntx->bufferMan->Free(rcv->buff);
        return ret;
    }

    std::vector<EasySerializeable*> ProcessReceived(const CryptData& crypt, const RawPacket* rcv)
    {
        std::vector<EasySerializeable*> ret{};
        EasyPacket pck(rcv->buff);
        if (pck.MakeDecrypted(crypt, rcv->buff))
        {
            decompressBuffer->~EasyBuffer();
            if (pck.MakeDecompressed(rcv->buff, decompressBuffer))
            {
                if (MakeDeserialized(decompressBuffer, ret))
                {

                }
            }
        }
        cntx->bufferMan->Free(rcv->buff);
        return ret;
    }

public:
    std::vector<EasySerializeable*> Process(const CryptData& crypt, EasyBuffer* rcvBuff)
    {
        std::vector<EasySerializeable*> ret{};
        EasyPacket pck(rcvBuff);
        if (pck.MakeDecrypted(crypt, rcvBuff))
        {
            decompressBuffer->reset();
            if (pck.MakeDecompressed(rcvBuff, decompressBuffer))
            {
                if (MakeDeserialized(decompressBuffer, ret))
                {

                }
            }
            cntx->bufferMan->Free(rcvBuff);
        }
        return ret;
    }
};

class SendManager {
    ServerContext* cntx;
    EasyBuffer* serializationBuffer;
    EasyBuffer* sendBuffer;

public:
    SendManager(ServerContext* cntx) : cntx(cntx), serializationBuffer(), sendBuffer()
    {
        while (!serializationBuffer)
            serializationBuffer = cntx->bufferMan->Get();
        while (!sendBuffer)
            sendBuffer = cntx->bufferMan->Get();
    }

    ~SendManager()
    {
        if(serializationBuffer)
            cntx->bufferMan->Free(serializationBuffer);
        if(sendBuffer)
            cntx->bufferMan->Free(sendBuffer);
    }

    void Update(double dt)
    {
        for (auto& [sid, cache] : cntx->sendCache)
        {
            SendMultiple(sid, cache);
            for (auto& obj : cache)
                delete obj;
            cache.clear();
        }
        cntx->sendCache.clear();
    }

private:
    void SendMultiple(SessionID_t sid, const std::vector<EasySerializeable*>& cache)
    {
        if (Session* session = cntx->sessionMan->GetSession(sid); ((cache.size() > 0U) && (session)))
        {
            serializationBuffer->reset();
            if (MakeSerialized(serializationBuffer, cache))
            {
                sendBuffer->reset();
                if (EasyPacket::MakeCompressed(serializationBuffer, sendBuffer))
                {
                    if (EasyPacket::MakeEncrypted({ sid, session->seqid_in, session->seqid_out, session->key }, sendBuffer))
                    {
                        if (uint64_t res = cntx->sck->send(sendBuffer->begin(), sendBuffer->m_payload_size + EasyPacket::HeaderSize(), session->addr); res == WSAEISCONN)
                        {
                            session->seqid_out++;
                        }
                    }
                }
            }
        }
    }

    void SendMultipleTo(SessionID_t sid, const std::vector<EasySerializeable*>& cache, const Addr_t& addr, const SequenceID_t& seqid_in, const SequenceID_t& seqid_out, const Key_t& key)
    {
        if (cache.size() > 0U)
        {
            serializationBuffer->reset();
            if (MakeSerialized(serializationBuffer, cache))
            {
                sendBuffer->reset();
                if (EasyPacket::MakeCompressed(serializationBuffer, sendBuffer))
                {
                    if (EasyPacket::MakeEncrypted({ sid, seqid_in, seqid_out, key }, sendBuffer))
                    {
                        if (uint64_t res = cntx->sck->send(sendBuffer->begin(), sendBuffer->m_payload_size + EasyPacket::HeaderSize(), addr); res == WSAEISCONN)
                        {

                        }
                    }
                }
            }
        }
    }

    friend class BaseServer;
};

BaseServer::BaseServer() : cntx(), running(false)
{

}

BaseServer::~BaseServer()
{
    Stop();
}

std::vector<EasySerializeable*> BaseServer::Process(const CryptData& crypt, EasyBuffer* buffer)
{
    return cntx->receiveMan->Process(crypt, buffer);
}

void BaseServer::Send(SessionID_t sid, const std::vector<EasySerializeable*>& objs, const Addr_t& addr, const SequenceID_t& seqid_in, const SequenceID_t& seqid_out, const Key_t& key)
{
    cntx->sendMan->SendMultipleTo(sid, objs, addr, seqid_in, seqid_out, key);
}

void BaseServer::Send(SessionID_t sid, const std::vector<EasySerializeable*>& objs)
{
    cntx->sendMan->SendMultiple(sid, objs);
}

void BaseServer::DestroySession(SessionID_t sid, std::string disconnectMessage)
{
    cntx->sessionMan->DestroySession(sid, disconnectMessage);
}

bool BaseServer::Start(EasyBufferManager* bm, const unsigned short port, ServerCallback* cbk, bool encryption, bool compression)
{
    if (IsRunning())
        return false;

    std::cout << "[BaseServer] Start - Starting...\n";
    cntx = new ServerContext(bm, cbk, encryption, compression);
    
    cntx->sck = new EasySocket();
    if (cntx->sck->bind(port, EasyIpAddress::Any) != WSAEISCONN)
    {
        std::cout << "[BaseServer] Start - Failed to bind the port!" << std::endl;
        return IsRunning();
    }

    running = true;
    BaseServer::instance = this;

    cntx->sessionMan = new SessionManager(cntx);
    cntx->receiveMan = new ReceiveManager(cntx);
    cntx->sendMan = new SendManager(cntx);

    if (!cntx->cbk->OnServerStart())
        Stop();
    else
        std::cout << "[BaseServer] Start - Started.\n";

    return IsRunning();
}

void BaseServer::Stop(std::string shutdownMessage)
{
    if (!IsRunning())
        return;
    std::cout << "[BaseServer] Stop - Stopping...\n";

    cntx->cbk->OnServerStop(shutdownMessage);

    running = false;
    BaseServer::instance = nullptr;

    delete cntx->sessionMan;
    cntx->sessionMan = nullptr;

    delete cntx->receiveMan;
    cntx->receiveMan = nullptr;

    delete cntx->sendMan;
    cntx->sendMan = nullptr;

    delete cntx->sck;
    cntx->sck = nullptr;

    delete cntx;
    cntx = nullptr;

    std::cout << "[BaseServer] Stop - Stopped.\n";
}

void BaseServer::Update(double dt)
{
    if (!IsRunning())
        return;

    cntx->receiveMan->Update(dt);
    cntx->sendMan->Update(dt);
    cntx->sessionMan->Update(dt);
}

bool BaseServer::IsRunning()
{
    return running;
}

ServerCache_t& BaseServer::GetReceiveCache()
{
    return cntx->receiveCache;
}

ServerCache_t& BaseServer::GetSendCache()
{
    return cntx->sendCache;
}