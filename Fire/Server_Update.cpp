#include "Server.hpp"
#include "World.hpp"



void Server::Update()
{
    Timestamp_t nextReceive = Clock::now();
    Timestamp_t nextUpdate = Clock::now();
    while (running)
    {
        Timestamp_t currentTime = Clock::now();
        if (nextReceive < currentTime)
        {
            nextReceive = currentTime + Millis_t(3U);

            // TODO

        }

        currentTime = Clock::now();
        if (nextUpdate < currentTime)
        {
            double elapsed = std::chrono::duration<double, std::milli>(currentTime - nextUpdate).count();
            nextUpdate = currentTime + Millis_t(16U);
            world->Update((elapsed / 1000.0));
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1U));
    }
}
