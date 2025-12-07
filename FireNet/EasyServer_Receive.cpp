#include "pch.h"
#include "EasyServer.hpp"

#include "EasySocket.hpp"
#include "EasyBuffer.hpp"
#include "EasyPacket.hpp"
#include "EasyDB.hpp"
#include "EasySerializer.hpp"

#include <unordered_map>
#include <array>
#include <sstream>


#ifdef SERVER_STATISTICS
#define STATS(x)                            stats_receive.x
class Statistics {
public:
    StatisticsCounter_t receive{};
    StatisticsCounter_t receiveFail{};
    StatisticsCounter_t drop{};
    StatisticsCounter_t dbKeyRead{};
    StatisticsCounter_t dbKeyFail{};
    StatisticsCounter_t deserialize{};
    StatisticsCounter_t sessionRead{};
    StatisticsCounter_t decryptFail{};
    StatisticsCounter_t decompressFail{};
    StatisticsCounter_t deserializeFail{};
    StatisticsCounter_t bmFail{};
    StatisticsCounter_t sessionFail{};
    StatisticsCounter_t sessionFailAddr{};
    StatisticsCounter_t sessionFailSequenceIDIn{};
    StatisticsCounter_t sessionFailAlive{};
};
Statistics stats_receive;
#else
#define STATS(x)                            stats_receive
StatisticsCounter_t stats_receive{};
#endif
#define STATS_LOAD(x)                       (STATS(x).load())
#define STATS_INCREASE(x)                   (++STATS(x))
#define STATS_INCREASE_X(x, y)              (STATS(x).fetch_add(y))

#define STATS_RECEIVED                      STATS_INCREASE(receive)
#define STATS_RECEIVE_FAIL                  STATS_INCREASE(receiveFail)
#define STATS_DROPPED                       STATS_INCREASE(drop)
#define STATS_DB_READ                       STATS_INCREASE(dbKeyRead)
#define STATS_DB_FAIL                       STATS_INCREASE(dbKeyFail)
#define STATS_DESERIALIZE                   STATS_INCREASE(deserialize)
#define STATS_SESSION_READ                  STATS_INCREASE(sessionRead)
#define STATS_DECRYPT_FAIL                  STATS_INCREASE(decryptFail)
#define STATS_DECOMPRESS_FAIL               STATS_INCREASE(decompressFail)
#define STATS_DESERIALIZE_FAIL              STATS_INCREASE(deserializeFail)
#define STATS_BM_FAIL                       STATS_INCREASE(bmFail)
#define STATS_SESSION_FAIL                  STATS_INCREASE(sessionFail)
#define STATS_SESSION_FAIL_ADDR             STATS_INCREASE(sessionFailAddr)
#define STATS_SESSION_FAIL_SEQ_IN           STATS_INCREASE(sessionFailSequenceIDIn)
#define STATS_SESSION_FAIL_ALIVE            STATS_INCREASE(sessionFailAlive)

class EasyServer_ReceviveHelper {
public:
    class RawBuffer {
    public:
        const SessionID_t sessionID;
        const Addr_t addr;
        const Timestamp_t receive;
        EasyBuffer* buffer;

        RawBuffer(const SessionID_t& sessionID, const Addr_t& addr, EasyBuffer* const buffer) : sessionID(sessionID), addr(addr), buffer(buffer), receive(Clock::now())
        {

        }

        RawBuffer(RawBuffer&& other) noexcept : sessionID(other.sessionID), addr(other.addr), receive(other.receive), buffer(other.buffer)
        {
            other.buffer = nullptr;
        }

        RawBuffer& operator=(RawBuffer&& other) noexcept
        {
            if (this != &other)
            {
                buffer = other.buffer;
                other.buffer = nullptr;
            }
            return *this;
        }

    };

    class R2PBuffer {
    public:
        class R2PRawBuffer {
        public:
            const Addr_t addr;
            const Timestamp_t receive;
            EasyBuffer* buffer;

            R2PRawBuffer(const RawBuffer& raw) : addr(raw.addr), receive(raw.receive), buffer(raw.buffer)
            {

            }
        };

        Session* session;
        std::vector<R2PRawBuffer> buffer;

        R2PBuffer(Session* session, const std::vector<RawBuffer>& raw) : session(session)
        {
            for (const RawBuffer& rawBuffer : raw)
                buffer.push_back(R2PRawBuffer(rawBuffer));
        }
    };

    class BaseCache_t {
    public:
        bool readKeyFromDBFlag = false;
        bool readKeyFromSessionFlag = true;
        std::vector<RawBuffer> cache;

    };

    using RawCacheType_t = std::unordered_map<SessionID_t, BaseCache_t>;
    using R2PCacheType_t = std::unordered_map<SessionID_t, R2PBuffer>;

    RawCacheType_t rawCache;
    RawCacheType_t unknownRawCache;
    R2PCacheType_t r2pCache;
    ObjCacheType_t internal_in_cache;
    LockedVec_t<Session*> newSessions;
    
    EasyServer* server;

    EasyServer_ReceviveHelper(EasyServer* server) : server(server)
    {

    }

    bool ReadKeyFromMYSQL(const SessionID_t& session_id, Key_t& key, UserID_t& user_id)
    {
        bool ret = false;
        sql::PreparedStatement* stmt = server->db->PrepareStatement("SELECT * FROM sessions WHERE id=? LIMIT 1;");
        stmt->setUInt(1, session_id);
        sql::ResultSet* res = stmt->executeQuery();
        while (res->next())
        {
            user_id = res->getUInt(2);
            memcpy(key.data(), res->getString(3).c_str(), KEY_SIZE);
            ret = true; // If valid_until < current_timestamp
            break;
        }
        delete res;
        delete stmt;
        return ret;
    }

    void BaseReceive()
    {
        EasyBuffer* receiveBuff = server->bf->Get();
        static Timestamp_t nextListen = Clock::now();
        Timestamp_t currentTime = Clock::now();
        if (nextListen < currentTime)
        {
            Timestamp_t endTime = nextListen + Millis_t(4U);
            nextListen = currentTime + Millis_t(9U);

            while (currentTime < endTime)
            {
                if (receiveBuff != nullptr)
                {
                    receiveBuff->reset();
                    Addr_t addr;
                    uint64_t ret = server->sock->receive(receiveBuff->begin(), receiveBuff->capacity(), receiveBuff->m_payload_size, addr);
                    if (ret == WSAEISCONN)
                    {
                        if (receiveBuff->m_payload_size >= EasyPacket::MinimumSize())
                        {
                            EasyPacket packet(receiveBuff);
                            SessionID_t sessionID = *packet.SessionID();
                            if (IS_SESSION(sessionID))
                            {
                                rawCache[sessionID].cache.emplace_back(sessionID, addr, receiveBuff);
                                receiveBuff = server->bf->Get();
                                STATS_RECEIVED;
                            }
                            else
                            {
                                STATS_DROPPED;
                            }
                        }
                        else
                        {
                            STATS_DROPPED;
                        }
                    }
                    else if (ret == WSAEWOULDBLOCK || ret == WSAEALREADY)
                    {
                        // nop
                    }
                    else
                    {
                        STATS_RECEIVE_FAIL;
                    }
                }
                else
                {
                    receiveBuff = server->bf->Get();
                    STATS_BM_FAIL;
                }
                currentTime = Clock::now();
            }
        }
        server->bf->Free(receiveBuff);
    }

    void Prepare()
    {
        static Timestamp_t nextPrepare = Clock::now();
        Timestamp_t currentTime = Clock::now();
        if (rawCache.size() > 0U && nextPrepare < currentTime)
        {
            nextPrepare = currentTime + Millis_t(5U);

            // 'rawCache' to ('r2pCache' or 'unknownRawCache')
            for (RawCacheType_t::iterator it = rawCache.begin(); it != rawCache.end(); ++it)
            {
                // If r2pCache cache exists for this ression add to its buffer
                if (auto res = r2pCache.find(it->first); res != r2pCache.end())
                {
                    // Sanity checks, alive, addr, session(maybe)
                    // Crypt data already exists, merge buffers
                    res->second.buffer.reserve(res->second.buffer.size() + it->second.cache.size());
                    std::transform(
                        it->second.cache.begin(),
                        it->second.cache.end(),
                        std::back_inserter(res->second.buffer),
                        [](const RawBuffer& rb) {
                            return R2PBuffer::R2PRawBuffer(rb);
                        }
                    );
                }
                else
                {
                    // Sanity checks, alive, addr, session(maybe)
                    // Crypt data for this list will be read from session, if session not exist then will read from database
                    auto [iter, inserted] = unknownRawCache.emplace(it->first, std::move(it->second));
                    if (!inserted)
                    {
                        // Key already existed, append buffers
                        iter->second.cache.insert(
                            iter->second.cache.end(),
                            std::make_move_iterator(it->second.cache.begin()),
                            std::make_move_iterator(it->second.cache.end())
                        );
                    }
                }
            }
            rawCache.clear();
        }
    }

    void ReadKeysFromDB()
    {
        static Timestamp_t nextRead = Clock::now();
        Timestamp_t currentTime = Clock::now();
        if (unknownRawCache.size() > 0U && nextRead < currentTime)
        {
            for (auto it = unknownRawCache.begin(); it != unknownRawCache.end(); )
            {
                nextRead = currentTime + Millis_t(100U);
                Key_t key(KEY_SIZE);
                if (it->second.cache.size() > 0U && it->second.readKeyFromDBFlag)
                {
                    UserID_t user_id;
                    if (ReadKeyFromMYSQL(it->first, key, user_id))
                    {
                        // Sanity checks, addr, sequence etc.
                        // Crypt data is ready, insert into r2pCache
                        SessionID_t sessionID = it->first;
                        Addr_t addr = it->second.cache.at(0).addr;
                        SequenceID_t sequenceID_in = 0U;
                        SequenceID_t sequenceID_out = 0U;
                        Session* newSession = new Session(it->first, user_id, addr, key, {}, sequenceID_in, sequenceID_out, Clock::now());
                        newSessions.vec.push_back(newSession);
                        r2pCache.emplace(
                            std::piecewise_construct,
                            std::forward_as_tuple(it->first),
                            std::forward_as_tuple(newSession, it->second.cache)
                        );
                        STATS_DB_READ;
                    }
                    else
                    {
                        // Not found in database, free the buffers
                        for (auto& b : it->second.cache)
                        delete b.buffer;
                        STATS_DB_FAIL;
                    }
                    it = unknownRawCache.erase(it);
                }
                else
                {
                    it++;
                }
            }
        }
    }

    void ReadKeysFromSessions()
    {
        static Timestamp_t nextRead = Clock::now();
        Timestamp_t currentTime = Clock::now();
        if (unknownRawCache.size() > 0U && nextRead < currentTime)
        {
            // Try read Crypt data from sessions
            if (server->m->sessionsMutex.try_lock())
            {
                nextRead = currentTime + Millis_t(10U);
                currentTime = Clock::now();
                for (auto it = unknownRawCache.begin(); it != unknownRawCache.end(); )
                {
                    Session* session = server->m->sessions[it->first];
                    if (it->second.readKeyFromSessionFlag && !it->second.readKeyFromDBFlag && session)
                    {
                        bool destroySession = false;
                        SessionStatus sessionStatus = SessionStatus::UNSET;
                        for (std::vector<RawBuffer>::iterator buffer = it->second.cache.begin(); buffer != it->second.cache.end(); ++buffer)
                        {
                            // If packet is arrived 'server->sessionTimeout' after last receive, close session because it already should be.
                            //bool alive = std::chrono::duration_cast<std::chrono::milliseconds>(buffer->receive - session->lastReceive) < std::chrono::milliseconds(server->sessionTimeout);
                            //if (!alive)
                            //{
                            //    STATS_SESSION_FAIL_ALIVE;
                            //    sessionStatus = SessionStatus::TIMED_OUT;
                            //    destroySession = true;
                            //    break;
                            //}
                            //else
                            {
                                session->lastReceive = buffer->receive;
                            }

                            // If addresses doesnt match, then free this packet
                            bool addr = buffer->addr == session->addr;
                            if (!addr)
                            {
                                STATS_SESSION_FAIL_ADDR;
                                sessionStatus = SessionStatus::ADDR_MISMATCH;
                                destroySession = true;
                                continue;
                            }

                            // If sequence id in didnt increase, then free this packet
                            //bool seq_in = *EasyPacket::SequenceID(buffer->buffer) > session->sequenceID_in;
                            //if (!seq_in)
                            //{
                            //    STATS_SESSION_FAIL_SEQ_IN;
                            //    sessionStatus = SessionStatus::SEQUENCE_MISMATCH;
                            //    destroySession = true;
                            //    continue;
                            //}
                        }
                        if (destroySession)
                        {
                            Addr_t respAddr;
                            if (it->second.cache.size() > 0)
                                respAddr = it->second.cache.at(0).addr;

                            Session* toDestroy = new Session(session->sid, session->uid, respAddr, session->key, {}, 0, 0, Clock::now());
                            sLoginResponse loginResponse = sLoginResponse(false, "User was already logged in! Try again!");
                            server->SendInstantPacket(toDestroy, { &loginResponse });
                            delete toDestroy;

                            for (RawBuffer& b : it->second.cache)
                            {
                                delete b.buffer;
                            }
                            server->DestroySession_internal(session, sessionStatus);
                            it = unknownRawCache.erase(it);
                            STATS_SESSION_FAIL;
                        }
                        else
                        {
                            session->seqid_in += static_cast<SequenceID_t>(it->second.cache.size());
                            r2pCache.emplace(it->first, R2PBuffer(session, it->second.cache)); // r2pCache cache is empty but we have session for Crypt data read from Session and removed from unknowns
                            it = unknownRawCache.erase(it);
                            STATS_SESSION_READ;
                        }
                    }
                    else
                    {
                        it->second.readKeyFromDBFlag = true;
                        ++it;
                    }
                }
                server->m->sessionsMutex.unlock();
            }
        }
    }

    void Process()
    {
        EasyBuffer* decompressionBuff = server->bf->Get();
        Timestamp_t nextProcess = Clock::now();
        Timestamp_t currentTime = Clock::now();
        if (r2pCache.size() > 0U && nextProcess < currentTime)
        {
            nextProcess = currentTime + Millis_t(5U);
            for (R2PCacheType_t::iterator it = r2pCache.begin(); it != r2pCache.end(); ++it)
            {
                decompressionBuff->reset();
                CryptData cryptInfo(it->first, it->second.session->seqid_in, 0U, it->second.session->key);
                for (std::vector<R2PBuffer::R2PRawBuffer>::iterator it2 = it->second.buffer.begin(); it2 != it->second.buffer.end(); ++it2)
                {
                    if (EasyPacket::MakeDecrypted(cryptInfo, it2->buffer))
                    {
                        if (EasyPacket::MakeDecompressed(it2->buffer, decompressionBuff))
                        {
                            if (MakeDeserialized(decompressionBuff, internal_in_cache[it->second.session->sid]))
                            {
                                STATS_DESERIALIZE;
                            }
                            else
                            {
                                STATS_DESERIALIZE_FAIL;
                            }
                        }
                        else
                        {
                            STATS_DECOMPRESS_FAIL;
                        }
                    }
                    else
                    {
                        STATS_DECRYPT_FAIL;
                    }
                    delete it2->buffer;
                    ++cryptInfo.sequence_id_in;
                }
            }
            r2pCache.clear();
        }
        server->bf->Free(decompressionBuff);
    }

    void Flush()
    {
        static Timestamp_t nextFlush = Clock::now();
        Timestamp_t currentTime = Clock::now();
        if (internal_in_cache.size() > 0U && nextFlush < currentTime)
        {
            if (server->m->in_cache.mutex.try_lock())
            {
                nextFlush = currentTime + Millis_t(5U);
                for (auto& [key, vec] : internal_in_cache)
                {
                    auto& targetVec = server->m->in_cache.cache[key];
                    targetVec.insert(targetVec.end(), vec.begin(), vec.end());
                }
                server->m->in_cache.mutex.unlock();
                internal_in_cache.clear();
            }
        }
    }

    void CreateSessions()
    {
        static Timestamp_t nextCreate = Clock::now();
        Timestamp_t currentTime = Clock::now();
        if (newSessions.vec.size() > 0U && nextCreate < currentTime)
        {
            if (server->m->sessionsMutex.try_lock())
            {
                nextCreate = currentTime + Millis_t(100U);
                for (Session* session : newSessions.vec)
                    server->CreateSession_internal(session);
                server->m->sessionsMutex.unlock();
                newSessions.vec.clear();
            }
        }
    }

    void Update()
    {
        // Receive into 'rawCache'. run every 9ms for 4ms
        BaseReceive();

        // Prepare, move 'rawCache' into 'r2pCache', run every 5ms
        Prepare();

        // 'unknownRawCache' to 'r2pCache' using Sessions, run every 10ms
        ReadKeysFromSessions();

        // 'unknownRawCache' to 'r2pCache' using database, run every 100ms
        ReadKeysFromDB();

        // Process 'r2pCache' into 'readObjCache', run every 5ms
        Process();

        // Create new Sessions, run every 100ms
        CreateSessions();

        // Flush, move 'internal_in_cache' into main context's 'in_cache', run every 5ms
        Flush();

        std::this_thread::sleep_for(std::chrono::milliseconds(1U));
    }
};

void EasyServer::Receive()
{
    EasyServer_ReceviveHelper helper(this);
    while (running)
    {
        helper.Update();
    }
}

std::string EasyServer::StatsReceive()
{
#ifdef SERVER_STATISTICS
    std::ostringstream ss;
    ss << "========= Server Receive Statistics ==========\n";
    ss << "    Receive:             " << STATS_LOAD(receive) << "\n";
    ss << "    Receive Fail:        " << STATS_LOAD(receiveFail) << "\n";
    ss << "    Drop:                " << STATS_LOAD(drop) << "\n";
    ss << "    DB Key Read:         " << STATS_LOAD(dbKeyRead) << "\n";
    ss << "    DB Key Read Fail:    " << STATS_LOAD(dbKeyFail) << "\n";
    ss << "    Deserialize:         " << STATS_LOAD(deserialize) << "\n";
    ss << "    Session Read:        " << STATS_LOAD(sessionRead) << "\n";
    ss << "    Decrypt Fail:        " << STATS_LOAD(decryptFail) << "\n";
    ss << "    Decompress Fail:     " << STATS_LOAD(decompressFail) << "\n";
    ss << "    Deserialize Fail:    " << STATS_LOAD(deserializeFail) << "\n";
    ss << "    Buffer Manager Fail: " << STATS_LOAD(bmFail) << "\n";
    ss << "    Session Fail:        " << STATS_LOAD(sessionFailAddr) << "\n";
    ss << "    Session Alive Fail:  " << STATS_LOAD(sessionFailAlive) << "\n";
    ss << "    Session Seq_in Fail: " << STATS_LOAD(sessionFailAddr) << "\n";
    ss << "    Session Addr Fail:   " << STATS_LOAD(sessionFailSequenceIDIn) << "\n";
    ss << "==============================================\n";
    return ss.str();
#else
    return "Server statistics disabled.\n";
#endif
}