#pragma once

#include <mutex>
#include <queue>
#include <map>
#include "EasyPeer.hpp"
#include "EasyNet.hpp"

class EasyManager {
public:
    using InBuffer_t = std::queue<std::map<EasyPeer, std::vector<EasyNetObj*>>>;
    struct In_t {
        InBuffer_t buffer;
        std::mutex lock;
    };
    bool running;
    std::thread loop;

    In_t in_buffer;

    EasyManager() : running(false)
    {

    }

    ~EasyManager()
    {
        Stop();
    }

    void Loop()
    {
        InBuffer_t cache;
        while (running)
        {
            if (in_buffer.lock.try_lock())
            {
                in_buffer.lock.unlock();
                std::this_thread::sleep_for(std::chrono::milliseconds(100U));
            }
        }
        Stop();
    }

    bool Start()
    {
        if (running)
            return false;
        running = true;
        loop = std::thread();
        return true;
    }

    bool Stop()
    {
        if (!running)
            return false;
        running = false;
        if (loop.joinable())
            loop.join();
        return true;
    }

};
