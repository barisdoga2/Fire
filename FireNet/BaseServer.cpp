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
#include <windows.h>
#include <ctime>

#include "EasyIpAddress.hpp"
#include "EasySocket.hpp"
#include "EasyBuffer.hpp"
#include "EasySerializer.hpp"
#include "EasyPacket.hpp"
#include "FireNet.hpp"

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

    void Update()
    {

    }

    Session* CreateSession(const SessionID_t& sid, const Addr_t& addr, const Timestamp_t& lastReceive)
    {
        if (sessions[sid])
            return nullptr;
        if (Session* session = new Session(sid, addr, {}, {}, 0U, 0U, lastReceive); cntx->cbk->OnSessionCreate(session->GetBase()))
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

    void DestroySession(SessionID_t sid)
    {
        if (!sessions[sid])
            return;
        cntx->cbk->OnSessionDestroy(sessions[sid]->GetBase());
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
        const SessionID_t sid;
        const Addr_t addr;
        EasyBuffer* buff;
        const Timestamp_t recv;
        RawPacket(const SessionID_t sid, const Addr_t addr, EasyBuffer* buff, const Timestamp_t recv) : sid(sid), addr(addr), buff(buff), recv(recv)
        {

        }
    };

    ServerContext* cntx;

public:
    ReceiveManager(ServerContext* cntx) : cntx(cntx)
    {

    }

    void Update()
    {
        if (RawPacket* rcv = ReceiveOne(); rcv)
        {
            Session* session = cntx->sessionMan->GetSession(rcv->sid);

            if (!session)
                session = cntx->sessionMan->CreateSession(rcv->sid, rcv->addr, rcv->recv);

            if (session)
                ProcessReceived(session, rcv);
        }
    }

private:
    ReceiveManager::RawPacket* ReceiveOne()
    {
        ReceiveManager::RawPacket* pck = nullptr;
        if (EasyBuffer* buff = cntx->bufferMan->Get(); buff)
        {
            uint64_t addr;
            if (uint64_t ret = cntx->sck->receive(buff->begin(), buff->capacity(), buff->m_payload_size, addr); ret == WSAEISCONN)
            {
                if (buff->m_payload_size >= EasyPacket::MinimumSize())
                {
                    EasyPacket packet(buff);
                    SessionID_t sid = *packet.SessionID();
                    if (IS_SESSION(sid))
                        pck = new ReceiveManager::RawPacket(sid, addr, buff, Clock::now());
                }
            }
            if (!pck)
                cntx->bufferMan->Free(buff);
        }
        return pck;
    }

    bool ProcessReceived(Session* session, RawPacket* rcv)
    {
        bool ret = false;
        EasyPacket pck(rcv->buff);
        CryptData crypt(session->sid, session->seqid_in, session->seqid_out, session->key);
        if (pck.MakeDecrypted(crypt, rcv->buff))
        {
            if (EasyBuffer* outBuff = cntx->bufferMan->Get(); outBuff)
            {
                if (pck.MakeDecompressed(rcv->buff, outBuff))
                {
                    std::vector<EasySerializeable*> cache;
                    if (MakeDeserialized(outBuff, cache))
                    {
                        cntx->receiveCache[session->sid].insert(cntx->receiveCache[session->sid].end(), cache.begin(), cache.end());
                        session->seqid_in++;
                        session->lastReceive = rcv->recv;
                        ret = true;
                    }
                }
                cntx->bufferMan->Free(outBuff);
            }
        }
        return ret;
    }
};

class SendManager {
    ServerContext* cntx;

public:
    SendManager(ServerContext* cntx) : cntx(cntx)
    {

    }

    void Update()
    {
        for (auto& [sid, cache] : cntx->sendCache)
            SendMultiple(sid, cache);
        cntx->sendCache.clear();
    }

private:
    void SendMultiple(SessionID_t sid, std::vector<EasySerializeable*>& cache)
    {
        if (Session* session = cntx->sessionMan->GetSession(sid); ((cache.size() > 0U) && (session)))
        {
            if (EasyBuffer* serializationBuffer = cntx->bufferMan->Get(); serializationBuffer)
            {
                if (MakeSerialized(serializationBuffer, cache))
                {
                    if (EasyBuffer* sendBuffer = cntx->bufferMan->Get(); sendBuffer)
                    {
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
                        cntx->bufferMan->Free(sendBuffer);
                    }
                }
                cntx->bufferMan->Free(serializationBuffer);
            }
        }
    }

};

BaseServer::BaseServer(EasyBufferManager* bm, const unsigned short port, ServerCallback* cbk, bool encryption, bool compression) : cntx(new ServerContext(bm, cbk, encryption, compression)), port(port), running(false)
{

}

BaseServer::~BaseServer()
{
    Stop();
}

bool BaseServer::Start()
{
    if (IsRunning())
        return false;
    std::cout << "[BaseServer] Starting...\n";
    cntx->sck = new EasySocket();
    if (cntx->sck->bind(port, EasyIpAddress::Any) != WSAEISCONN)
    {
        std::cout << "[BaseServer] Failed to bind the port!" << std::endl;
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
        std::cout << "[BaseServer] Started.\n";

    return IsRunning();
}

void BaseServer::Stop()
{
    if (!IsRunning())
        return;
    std::cout << "[BaseServer] Stopping...\n";

    cntx->cbk->OnServerStop();

    running = false;
    BaseServer::instance = nullptr;
    delete cntx;

    std::cout << "[BaseServer] Stopped.\n";
}

void BaseServer::Update()
{
    if (!IsRunning())
        return;

    cntx->receiveMan->Update();
    cntx->sendMan->Update();
    cntx->sessionMan->Update();

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