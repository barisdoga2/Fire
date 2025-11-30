#include "pch.h"
#include <sstream>

#include "EasyServer.hpp"
#include "EasyNet.hpp"
#include "EasySerializer.hpp"

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

class EasyServer_UpdateHelper {
public:
    EasyServer* server;

    EasyServer_UpdateHelper(EasyServer* server) : server(server)
    {

    }

    void UpdateIncomingObjCache()
    {
        static Timestamp_t nextUpdate = Clock::now();
        Timestamp_t currentTime = Clock::now();
        if (nextUpdate < currentTime)
        {
            if (server->m->in_cache.mutex.try_lock())
            {
                nextUpdate = currentTime + Millis_t(4U);

                for (auto& [sessionID, cache] : server->m->in_cache.cache)
                {
                    auto& dstVec = internal_in_cache[sessionID];
                    dstVec.insert(dstVec.end(), std::make_move_iterator(cache.begin()), std::make_move_iterator(cache.end()));
                }

                size_t size = server->m->in_cache.cache.size();
                server->m->in_cache.cache.clear();
                server->m->in_cache.mutex.unlock();
                STATS_IN(size);
            }
        }
    }

    void UpdateOutgoingObjCache()
    {
        static Timestamp_t nextUpdate = Clock::now();
        Timestamp_t currentTime = Clock::now();
        if (nextUpdate < currentTime)
        {
            if (server->m->out_cache.mutex.try_lock())
            {
                nextUpdate = currentTime + Millis_t(4U);

                for (auto& [sessionID, cache] : internal_out_cache)
                {
                    auto& dstVec = server->m->out_cache.cache[sessionID];
                    dstVec.insert(dstVec.end(), std::make_move_iterator(cache.begin()), std::make_move_iterator(cache.end()));
                }

                server->m->out_cache.mutex.unlock();
                size_t size = internal_out_cache.size();
                internal_out_cache.clear();
                STATS_OUT(size);
            }
        }
    }
};

void EasyServer::Update()
{
    EasyServer_UpdateHelper helper(this);
    while (running)
    {
        helper.UpdateIncomingObjCache();

        DoProcess(internal_in_cache, internal_out_cache);

        helper.UpdateOutgoingObjCache();

        std::this_thread::sleep_for(std::chrono::milliseconds(10U));
    }
}

std::string EasyServer::StatsUpdate()
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