#include "Server.hpp"
#include "World.hpp"
#include "Net.hpp"
#include <sstream>

#ifdef SERVER_STATISTICS
#define STATS(x)                            stats_update.x
class Statistics {
public:
    StatisticsCounter_t in{};
    StatisticsCounter_t out{};
    StatisticsCounter_t processed{};
    StatisticsCounter_t unprocessed{};
};
Statistics stats_update;
#else
#define STATS(x)                            stats_update
StatisticsCounter_t stats_update{};
#endif
#define STATS_LOAD(x)                       (STATS(x).load())
#define STATS_INCREASE(x)                   (++STATS(x))
#define STATS_INCREASE_X(x, y)              (STATS(x).fetch_add(y))

#define STATS_IN(x)                         STATS_INCREASE_X(in, x)
#define STATS_OUT(x)                        STATS_INCREASE_X(out, x)
#define STATS_PROCESSED                     STATS_INCREASE(processed)
#define STATS_UNPROCESSED                   STATS_INCREASE(unprocessed)

ObjCacheType_t internal_in_cache;
ObjCacheType_t internal_out_cache;

namespace Server_Update_internal {

    void UpdateIncomingObjCache(MainContex* m)
    {
        static Timestamp_t nextUpdate = Clock::now();
        Timestamp_t currentTime = Clock::now();
        if (nextUpdate < currentTime)
        {
            if (m->in_cache.mutex.try_lock())
            {
                nextUpdate = currentTime + Millis_t(4U);
                
                for (auto& [sessionID, cache] : m->in_cache.cache)
                {
                    auto& dstVec = internal_in_cache[sessionID];
                    dstVec.insert(dstVec.end(), std::make_move_iterator(cache.begin()), std::make_move_iterator(cache.end()));
                }

                size_t size = m->in_cache.cache.size();
                m->in_cache.cache.clear();
                m->in_cache.mutex.unlock();
                STATS_IN(size);
            }
        }
    }

    void UpdateOutgoingObjCache(MainContex* m)
    {
        static Timestamp_t nextUpdate = Clock::now();
        Timestamp_t currentTime = Clock::now();
        if (nextUpdate < currentTime)
        {
            if (m->out_cache.mutex.try_lock())
            {
                nextUpdate = currentTime + Millis_t(4U);

                for (auto& [sessionID, cache] : internal_out_cache)
                {
                    auto& dstVec = m->out_cache.cache[sessionID];
                    dstVec.insert(dstVec.end(), std::make_move_iterator(cache.begin()), std::make_move_iterator(cache.end()));
                }

                m->out_cache.mutex.unlock();
                size_t size = internal_out_cache.size();
                internal_out_cache.clear();
                STATS_OUT(size);
            }
        }
    }

    void ProcessObjCache()
    {
        for (ObjCacheType_t::iterator it = internal_in_cache.begin() ; it != internal_in_cache.end() ; ++it)
        {
            for (EasySerializeable* in_obj : it->second)
            {
                if (pHello* hello = dynamic_cast<pHello*>(in_obj); hello)
                {
                    if (true/*ECHO*/)
                    {
                        static int i = 0;
                        pHello* echo_hello = new pHello("Hi! I'm server." + std::to_string(++i) + ". You said: " + hello->message);
                        if (auto res = internal_out_cache.find(it->first); res != internal_out_cache.end())
                        {
                            res->second.push_back(echo_hello);
                        }
                        else
                        {
                            internal_out_cache.emplace(it->first, std::vector<EasySerializeable*>{ echo_hello });
                        }
                    }
                    STATS_PROCESSED;
                }
                else
                {
                    STATS_UNPROCESSED;
                }
                delete in_obj;
            }
        }
        internal_in_cache.clear();
    }
}

void Server::Update()
{
    using namespace Server_Update_internal;
    while (running)
    {
        UpdateIncomingObjCache(m);
        ProcessObjCache();
        UpdateOutgoingObjCache(m);
    }
}

std::string Server::StatsUpdate()
{
#ifdef SERVER_STATISTICS
    std::ostringstream ss;
    ss << "========= Server Update Statistics ==========\n";
    ss << "    In Objs:                 " << STATS_LOAD(in) << "\n";
    ss << "    Out Objs:                " << STATS_LOAD(out) << "\n";
    ss << "    Processed Objs:          " << STATS_LOAD(processed) << "\n";
    ss << "    Unprocessed Objs:        " << STATS_LOAD(unprocessed) << "\n";
    ss << "==============================================\n";
    return ss.str();
#else
    return "Server statistics disabled.\n";
#endif
}