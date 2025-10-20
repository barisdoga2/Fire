#include "Server.hpp"



void Server::Send()
{
    EasyBuffer* imBuff = bf->Get(true);
    EasyBuffer* imBuff2 = bf->Get(true);

    Timestamp_t currentTime = Clock::now();
    Timestamp_t nextClear = currentTime, endTime;

    //EasyNetObj_Buffer sendCache;
    while (running)
    {
        // Move some other cache into sendCache periodically, store actual Peers there
        // ...
        // ...
        // ...

        // Flush Send Cache
        endTime = currentTime + Millis_t(3U);
        //while (currentTime < endTime && sendCache.buffer.size() > 0U)
        //{
        //    for (auto& it : sendCache.buffer)
        //    {
        //        imBuff->reset();
        //        imBuff2->reset();

        //        std::vector<EasyNetObj>& objs = it.second;
        //        PeerSessionInfo session = it.first;
        //        
        //        if (objs.size() > 0U) // sanity check
        //        {
        //            uint64_t res = 0;

        //            bool status = true;
        //            if (status)
        //                status = MakeSerialized(imBuff, objs);

        //            if (status)
        //                status = EasyPacket::MakeCompressed(imBuff, imBuff2);

        //            if (status)
        //                status = EasyPacket::MakeEncrypted(session.crypt, imBuff2);

        //            if (status)
        //                res = sock->send(imBuff2->begin(), imBuff2->m_payload_size + EasyPacket::HeaderSize(), session.sock->addr);

        //            std::cout << "Server | Send! Result: " << res << ", Status: " << status << std::endl;
        //            objs.clear();
        //        }
        //    }
        //    sendCache.buffer.clear();
        //    currentTime = Clock::now();
        //}

        // Rest a bit
        std::this_thread::sleep_for(std::chrono::milliseconds(1U));
    }
}
