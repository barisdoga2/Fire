#include "Server.hpp"

#include "EasySocket.hpp"
#include "EasyBuffer.hpp"
#include "EasyPacket.hpp"
#include "EasyDB.hpp"
#include "World.hpp"
#include "Serializer.hpp"

#include <unordered_map>
#include <array>


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
    class R2PData {
    public:
        const SessionID_t sessionID;
        const Addr_t addr;
        const Key_t key;
        SequenceID_t sequenceID_in;

        R2PData(const SessionID_t& sessionID, const Addr_t& addr, const Key_t& key, const SequenceID_t& sequenceID_in = 0U) : sessionID(sessionID), addr(addr), key(key), sequenceID_in(sequenceID_in)
        {

        }

        R2PData(const Session* const session) : sessionID(session->sessionID), addr(session->addr), key(session->key), sequenceID_in(session->sequenceID_in)
        {

        }
    };

    class R2PRawBuffer {
    public:
        const Addr_t addr;
        const Timestamp_t receive;
        EasyBuffer* buffer;

        R2PRawBuffer(const RawBuffer& raw) : addr(raw.addr), receive(raw.receive), buffer(raw.buffer)
        {

        }
    };

    const R2PData data;
    std::vector<R2PRawBuffer> buffer;

    R2PBuffer(const R2PData& data, const std::vector<RawBuffer>& raw) : data(data)
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
ROCacheType_t internal_readObjCache;

bool ReadKeyFromMYSQL(EasyDB* db, const SessionID_t& session_id, Key_t& key)
{
    bool ret = false;
    sql::PreparedStatement* stmt = db->PrepareStatement("SELECT * FROM sessions WHERE id=? LIMIT 1;");
    stmt->setUInt(1, session_id);
    sql::ResultSet* res = stmt->executeQuery();
    while (res->next())
    {
        memcpy(key.data(), res->getString(3).c_str(), KEY_SIZE);
        ret = true; // If valid_until < current_timestamp
        break;
    }
    delete res;
    delete stmt;
    return ret;
}

void BaseReceive(EasySocket* sock, EasyBufferManager* bf)
{
    static EasyBuffer* receiveBuff = bf->Get();
    static Timestamp_t nextListen = Clock::now();
    Timestamp_t currentTime = Clock::now();
    if (nextListen < currentTime)
    {
        Timestamp_t endTime = nextListen + Millis_t(3U);
        nextListen = currentTime + Millis_t(9U);

        PeerSocketInfo inSockInfo;
        while (currentTime < endTime)
        {
            if (receiveBuff != nullptr)
            {
                receiveBuff->reset();
                uint64_t ret = sock->receive(receiveBuff->begin(), receiveBuff->capacity(), receiveBuff->m_payload_size, inSockInfo);
                if (ret == WSAEISCONN && receiveBuff->m_payload_size >= EasyPacket::MinimumSize())
                {
                    EasyPacket packet(receiveBuff);
                    SessionID_t sessionID = *packet.SessionID();
                    if (IS_SESSION(sessionID))
                    {
                        rawCache[sessionID].cache.emplace_back(sessionID, inSockInfo.addr, receiveBuff);
                        receiveBuff = bf->Get();
                    }
                }
            }
            else
            {
                receiveBuff = bf->Get();
            }
            currentTime = Clock::now();
        }
    }
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

void ReadKeysFromDB(EasyDB* db, EasyBufferManager* bf)
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
                if (ReadKeyFromMYSQL(db, it->first, key))
                {
                    // Sanity checks, addr, sequence etc.
                    // Crypt data is ready, insert into r2pCache
                    r2pCache.emplace(
                        std::piecewise_construct,
                        std::forward_as_tuple(it->first),
                        std::forward_as_tuple(R2PBuffer::R2PData(it->first, it->second.cache.at(0).addr, key, 0U), it->second.cache)
                    );
                }
                else
                {
                    // Not found in database, free the buffers
                    for (auto& b : it->second.cache)
                        bf->Free(b.buffer);
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

void ReadKeysFromSessions(MainContex* m)
{
    static Timestamp_t nextRead = Clock::now();
    Timestamp_t currentTime = Clock::now();
    if (unknownRawCache.size() > 0U && nextRead < currentTime)
    {
        // Try read Crypt data from sessions
        if (m->sessionsMutex.try_lock())
        {
            nextRead = currentTime + Millis_t(10U);
            for (auto it = unknownRawCache.begin(); it != unknownRawCache.end(); )
            {
                Session* session = m->sessions[it->first];
                if (it->second.readKeyFromSessionFlag && !it->second.readKeyFromDBFlag && session)
                {
                    // Sanity checks, addr, sequence etc.
                    r2pCache.emplace(it->first, R2PBuffer(session, it->second.cache)); // r2pCache cache is empty but we have session for Crypt data read from Session and removed from unknowns
                    it = unknownRawCache.erase(it);
                }
                else
                {
                    it->second.readKeyFromDBFlag = true;
                    ++it;
                }
            }
            m->sessionsMutex.unlock();
        }
    }
}

void Process(EasyBufferManager* bf)
{
    static EasyBuffer* decompressionBuff = bf->Get(true);
    static Timestamp_t nextProcess = Clock::now();
    Timestamp_t currentTime = Clock::now();
    if (r2pCache.size() > 0U && nextProcess < currentTime)
    {
        nextProcess = currentTime + Millis_t(5U);
        for (R2PCacheType_t::iterator it = r2pCache.begin(); it != r2pCache.end(); ++it)
        {
            PeerCryptInfo cryptInfo(it->first, it->second.data.sequenceID_in, 0U, it->second.data.key);
            for (std::vector<R2PBuffer::R2PRawBuffer>::iterator it2 = it->second.buffer.begin(); it2 != it->second.buffer.end(); ++it2)
            {
                bool status = false;
                uint8_t error_code = 0U;
                if (EasyPacket::MakeDecrypted(cryptInfo, it2->buffer))
                {
                    if (EasyPacket::MakeDecompressed(it2->buffer, decompressionBuff))
                    {
                        if (MakeDeserialized(decompressionBuff, internal_readObjCache[it->first]))
                        {
                            status = true;
                        }
                        else
                        {
                            error_code = 2U;
                        }
                    }
                    else
                    {
                        error_code = 2U;
                    }
                }
                else
                {
                    error_code = 1U;
                }
                ++cryptInfo.sequence_id_in;
                decompressionBuff->reset();
            }
        }
        r2pCache.clear();
    }
}

void Flush(MainContex* m)
{
    static Timestamp_t nextFlush = Clock::now();
    Timestamp_t currentTime = Clock::now();
    if (internal_readObjCache.size() > 0U && nextFlush < currentTime)
    {
        if (m->readyObjsMutex.try_lock())
        {
            nextFlush = currentTime + Millis_t(5U);
            for (auto& [key, vec] : internal_readObjCache)
            {
                auto& targetVec = m->readyObjsCache[key];
                targetVec.insert(targetVec.end(), vec.begin(), vec.end());
            }
            m->readyObjsMutex.unlock();
            internal_readObjCache.clear();
        }
    }
}

void Server::Receive()
{
    while (running)
    {
        // Receive into 'rawCache'. run every 9ms for 3ms
        BaseReceive(sock, bf);

        // Prepare, move 'rawCache' into 'r2pCache', run every 5ms
        Prepare();

        // 'unknownRawCache' to 'r2pCache' using Sessions, run every 10ms
        ReadKeysFromSessions(m);

        // 'unknownRawCache' to 'r2pCache' using database, run every 100ms
        ReadKeysFromDB(db, bf);

        // Process 'r2pCache' into 'readObjCache', run every 5ms
        Process(bf);

        // Flush, move 'internal_readObjCache' into main context's 'internal_readObjCache', run every 5ms
        Flush(m);
    }
}
