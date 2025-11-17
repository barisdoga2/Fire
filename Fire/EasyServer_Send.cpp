#include <sstream>

#include "EasyServer.hpp"
#include "EasyPacket.hpp"
#include "EasySocket.hpp"
#include "../Game Server/Serializer.hpp"

#ifdef SERVER_STATISTICS
#define STATS(x)                            stats_send.x
class Statistics {
public:
    StatisticsCounter_t send{};
    StatisticsCounter_t sendFail{};
    StatisticsCounter_t sessionRead{};
    StatisticsCounter_t encryped{};
    StatisticsCounter_t encryptFail{};
    StatisticsCounter_t compressFail{};
    StatisticsCounter_t serializeFail{};
    StatisticsCounter_t bmFail{};
};
Statistics stats_send;
#else
#define STATS(x)                            stats_send
StatisticsCounter_t stats_send{};
#endif
#define STATS_LOAD(x)                       (STATS(x).load())
#define STATS_INCREASE(x)                   (++STATS(x))
#define STATS_INCREASE_X(x, y)              (STATS(x).fetch_add(y))

#define STATS_SEND                          STATS_INCREASE(send)
#define STATS_SEND_FAIL                     STATS_INCREASE(sendFail)
#define STATS_SESSION_READ                  STATS_INCREASE(sessionRead)
#define STATS_ENCRYPTED                     STATS_INCREASE(encryped)
#define STATS_ENCRYPT_FAIL                  STATS_INCREASE(encryptFail)
#define STATS_COMPRESS_FAIL                 STATS_INCREASE(compressFail)
#define STATS_SERIALIZE_FAIL                STATS_INCREASE(serializeFail)
#define STATS_BM_FAIL                       STATS_INCREASE(bmFail)

namespace Server_Send_internal {
    class R2PBuffer {
    public:
        class R2PData {
        public:
            const SessionID_t sessionID;
            const Addr_t addr;
            const Key_t key;
            const Timestamp_t send;
            SequenceID_t sequenceID_out;

            R2PData(const SessionID_t& sessionID, const Addr_t& addr, const Key_t& key, const SequenceID_t& sequenceID_out = 0U) : sessionID(sessionID), addr(addr), key(key), sequenceID_out(sequenceID_out), send(Clock::now())
            {

            }

            R2PData(const Session* const session) : sessionID(session->sessionID), addr(session->addr), key(session->key), sequenceID_out(session->sequenceID_out)
            {

            }
        };

        const R2PData data;
        std::vector<EasySerializeable*> buffer;

        R2PBuffer(const R2PData& data, const std::vector<EasySerializeable*>& raw) : data(data)
        {
            buffer.reserve(buffer.size() + raw.size());
            buffer.insert(buffer.end(), raw.begin(), raw.end());
        }
    };

    using ReadyCacheBase_t = std::vector<EasyBuffer*>;
    using ReadyBuffer_t = std::unordered_map<SessionID_t, R2PBuffer>;
    using R2PCacheType_t = std::unordered_map<SessionID_t, R2PBuffer>;
    using ReadyCache_t = std::unordered_map<Addr_t, ReadyCacheBase_t>;

    ObjCacheType_t internal_out_cache;
    R2PCacheType_t r2pCache;
    ReadyCache_t internal_ready_cache;

    void BaseMove(MainContex* m)
    {
        static Timestamp_t nextMove = Clock::now();
        Timestamp_t currentTime = Clock::now();
        if (nextMove < currentTime)
        {
            if (m->out_cache.mutex.try_lock())
            {
                if (m->out_cache.cache.size() > 0U)
                {
                    nextMove = currentTime + Millis_t(3U);
                    for (auto& [key, vec] : m->out_cache.cache)
                    {
                        auto& targetVec = internal_out_cache[key];
                        targetVec.insert(targetVec.end(), vec.begin(), vec.end());
                    }
                    m->out_cache.cache.clear();
                }
                m->out_cache.mutex.unlock();
            }
        }
    }

    void Prepare(MainContex* m)
    {
        static Timestamp_t nextRead = Clock::now();
        Timestamp_t currentTime = Clock::now();
        if (internal_out_cache.size() > 0U && nextRead < currentTime)
        {
            // Try read Crypt data from sessions
            if (m->sessionsMutex.try_lock())
            {
                nextRead = currentTime + Millis_t(4U);
                for (auto it = internal_out_cache.begin(); it != internal_out_cache.end(); ++it)
                {
                    Session* session = m->sessions[it->first];
                    if (session)
                    {
                        r2pCache.emplace(it->first, R2PBuffer(session, it->second));
                    }
                    else
                    {
                        for (EasySerializeable* s : it->second)
                            delete s;
                    }
                }
                m->sessionsMutex.unlock();
                internal_out_cache.clear();
            }
        }
    }

    void Process(EasyBufferManager* bf)
    {
        static EasyBuffer* serializationBuff = bf->Get(true);
        static EasyBuffer* sendBuff = bf->Get(true);
        static Timestamp_t nextProcess = Clock::now();
        Timestamp_t currentTime = Clock::now();
        if (r2pCache.size() > 0U && nextProcess < currentTime)
        {
            nextProcess = currentTime + Millis_t(5U);
            for (R2PCacheType_t::iterator it = r2pCache.begin(); it != r2pCache.end(); ++it)
            {
                if (sendBuff != nullptr)
                {
                    sendBuff->reset();
                    serializationBuff->reset();

                    PeerCryptInfo cryptInfo(it->first, it->second.data.sequenceID_out, 0U, it->second.data.key);
                    if (MakeSerialized(serializationBuff, it->second.buffer))
                    {
                        if (EasyPacket::MakeCompressed(serializationBuff, sendBuff))
                        {
                            if (EasyPacket::MakeEncrypted(cryptInfo, sendBuff))
                            {
                                if (auto res = internal_ready_cache.find(it->second.data.addr); res != internal_ready_cache.end())
                                {
                                    res->second.push_back(sendBuff);
                                }
                                else
                                {
                                    internal_ready_cache.emplace(it->second.data.addr, std::vector<EasyBuffer*>{ sendBuff });
                                }
                                sendBuff = bf->Get();
                                STATS_ENCRYPTED;
                            }
                            else
                            {
                                STATS_ENCRYPT_FAIL;
                            }
                        }
                        else
                        {
                            STATS_COMPRESS_FAIL;
                        }
                    }
                    else
                    {
                        STATS_SERIALIZE_FAIL;
                    }
                    ++cryptInfo.sequence_id_out;
                }
                else
                {
                    STATS_BM_FAIL;
                    sendBuff = bf->Get();
                }
                
            }
            r2pCache.clear();
        }
    }

    void Flush(EasySocket* sock, EasyBufferManager* bf)
    {
        static Timestamp_t nextFlush = Clock::now();
        Timestamp_t currentTime = Clock::now();
        if (internal_ready_cache.size() > 0U && nextFlush < currentTime)
        {
            nextFlush = currentTime + Millis_t(5U);
            for (auto& [addr, vec] : internal_ready_cache)
            {
                for (EasyBuffer* buf : vec)
                {
                    uint64_t res = sock->send(buf->begin(), buf->m_payload_size + EasyPacket::HeaderSize(), addr);
                    if (res != WSAEISCONN)
                    {
                        STATS_SEND_FAIL;
                    }
                    else
                    {
                        STATS_SEND;
                    }
                    bf->Free(buf);
                }
            }
            internal_ready_cache.clear();
        }
    }
}

void EasyServer::Send()
{
    using namespace Server_Send_internal;

    while (running)
    {
        // Move main context's 'out_cache' into 'internal_out_cache', run every 4ms
        BaseMove(m);

        // Prepare, move 'internal_out_cache' into 'r2pCache', get crypt data from sessions, run every 4ms
        Prepare(m);

        // Process 'r2pCache' into 'internal_ready_cache', run every 5ms
        Process(bf);

        // Flush, send 'internal_ready_cache' to clients, run every 6ms for 3ms
        Flush(sock, bf);
    }
}

std::string EasyServer::StatsSend()
{
#ifdef SERVER_STATISTICS
    std::ostringstream ss;
    ss << "========= Server Send Statistics ==========\n";
    ss << "    Send:                " << STATS_LOAD(send) << "\n";
    ss << "    Send Fail:           " << STATS_LOAD(sendFail) << "\n";
    ss << "    Encrypted:           " << STATS_LOAD(encryped) << "\n";
    ss << "    Session Read:        " << STATS_LOAD(sessionRead) << "\n";
    ss << "    Encrypt Fail:        " << STATS_LOAD(encryptFail) << "\n";
    ss << "    Compress Fail:       " << STATS_LOAD(compressFail) << "\n";
    ss << "    Serialize Fail:      " << STATS_LOAD(serializeFail) << "\n";
    ss << "    Buffer Manager Fail: " << STATS_LOAD(bmFail) << "\n";
    ss << "==============================================\n";
    return ss.str();
#else
    return "Server statistics disabled.\n";
#endif
}