#pragma once

#include <string>
#include <iostream>
#include <vector>
#include <EasySocket.hpp>
#include <EasyBuffer.hpp>
#include "EasyNet.hpp"
#include "Config.hpp"
#include "../FireServer/ServerNet.hpp"
#include <EasyIpAddress.hpp>

class ClientSession {
public:
    std::string username{}, password{};
    SessionID_t sid{};
    Addr_t addr{};
    Key_t key{};
    Key_Expt_t key_expiry{};
    SequenceID_t seqid_in{}, seqid_out{};
    Timestamp_t lastReceive{};

    UserID_t uid{};
    EasySocket sck{};
    std::mutex cacheM{};
    std::vector<EasySerializeable*> receiveCache{};
    std::vector<EasySerializeable*> sendCache{};
    UserStats stats{};

    ClientSession() 
    {

    }

    void Stop()
    {
        sid = 0U;
        addr = 0U;
        key = {};
        key_expiry = {};
        seqid_in = 0U;
        seqid_out = 0U;
        lastReceive = {};

        uid = 0U;
        sck = {};
        receiveCache.clear();
        sendCache.clear();

        stats.uid = 0U;
        stats.gametime = 0U;
        stats.golds = 0U;
        stats.diamonds = 0U;
        stats.tutorial_done = 0U;
        stats.champions_owned = {};
    }
};

class ClientNetwork {
    EasyBufferManager* bufferMan;
    bool isInGame{};
    bool isLoggingIn{};
    bool isLoginFailed{};
    bool isChampionSelect{};
    std::string loginStatus{};

public:
    ClientSession session;

    ClientNetwork(EasyBufferManager* bufferMan) : bufferMan(bufferMan), session()
    {

    }

    void Disconnect(std::string serverMessage = "")
    {
        isInGame = false;
        isLoggingIn = false;

        Stop();

        loginStatus = serverMessage;
        isLoginFailed = true;
    }

    void Stop()
    {
        if (isInGame || isLoggingIn)
        {
            session.sendCache.push_back(new sLogoutRequest());
            SendOne();
            std::cout << "Logout request sent!\n";
        }
        
        isInGame = false;
        isLoggingIn = false;
        isLoginFailed = false;
        isChampionSelect = false;
        loginStatus = "";

        session.Stop();
    }

    void Update()
    {
        if (!isInGame)
            return;

        // Heartbeat
        {
            static Timestamp_t nextHeartbeat = Clock::now();
            Millis_t heartbeatPeriod(1000U);

            if (Clock::now() >= nextHeartbeat)
            {
                nextHeartbeat = Clock::now() + heartbeatPeriod;

                std::cout << "[ClientNetwork] Update - Heartbeat sent.\n";
                session.sendCache.push_back(new sHearbeat());
                SendOne();
            }
        }

        ReceiveOne();
        SendOne();
    }

    void Login(std::string url)
    {
        Stop();

        isLoggingIn = true;

        if (Auth(url))
        {
            loginStatus = "Waiting game server...";
            std::cout << "[ClientNetwork] Login - Login request sent.\n";
            session.sendCache.push_back(new sLoginRequest());
            SendOne();

            bool to{};
            Millis_t timeout = Millis_t(3000U);
            Timestamp_t nextHeartbeat = Clock::now();
            Timestamp_t end = Clock::now() + timeout;
            std::string timeoutStatus = "Login response timed out!";
            while (isLoggingIn && !isInGame && !to)
            {
                to = Clock::now() > end;
                ReceiveOne();
                std::vector<EasySerializeable*>& cache = GetReceiveCache();
                for (std::vector<EasySerializeable*>::iterator objIt = cache.begin(); objIt != cache.end() && isLoggingIn && !isInGame; )
                {
                    if (sLoginResponse* loginResp = dynamic_cast<sLoginResponse*>(*objIt); loginResp)
                    {
                        end = Clock::now() + timeout;
                        timeoutStatus = "Player boot info timed out!";

                        std::cout << "[ClientNetwork] Login - sLoginResponse received.\n";
                        loginStatus = loginResp->message;

                        delete* objIt;
                        objIt = cache.erase(objIt);
                    }
                    else if (sPlayerBootInfo* bootInfo = dynamic_cast<sPlayerBootInfo*>(*objIt); bootInfo)
                    {
                        end = Clock::now() + timeout;
                        timeoutStatus = "Champion select response timed out!";

                        std::cout << "[ClientNetwork] Login - sPlayerBootInfo received.\n";
                        loginStatus = "PlayerBootInfo received.";
                        isChampionSelect = true;

                        session.stats.uid = bootInfo->userID;
                        session.stats.gametime = bootInfo->gametime;
                        session.stats.golds = bootInfo->golds;
                        session.stats.diamonds = bootInfo->diamonds;
                        session.stats.tutorial_done = bootInfo->tutorialDone;
                        session.stats.champions_owned = bootInfo->championsOwned;

                        delete* objIt;
                        objIt = cache.erase(objIt);
                    }
                    else if (sChampionSelectResponse* championSelectResp = dynamic_cast<sChampionSelectResponse*>(*objIt); championSelectResp)
                    {
                        end = Clock::now() + timeout;
                        timeoutStatus = "Game boot timed out!";

                        std::cout << "[ClientNetwork] Login - sChampionSelectResponse received.\n";
                        loginStatus = championSelectResp->message;
                        isLoggingIn = championSelectResp->response;

                        if (isLoggingIn)
                            session.sendCache.push_back(new sHearbeat());

                        delete* objIt;
                        objIt = cache.erase(objIt);
                    }
                    else if (sGameBoot* gameBoot = dynamic_cast<sGameBoot*>(*objIt); gameBoot)
                    {
                        end = Clock::now() + timeout;
                        timeoutStatus = "Heartbeat timed out!";
                        isInGame = true;

                        std::cout << "[ClientNetwork] Login - sGameBoot received.\n";
                        session.sendCache.push_back(new sHearbeat());
                        SendOne();

                        delete* objIt;
                        objIt = cache.erase(objIt);
                    }
                    else if (sHearbeat* heartbeat = dynamic_cast<sHearbeat*>(*objIt); heartbeat)
                    {
                        end = Clock::now() + timeout;
                        timeoutStatus = "Heartbeat timed out!";

                        std::cout << "[ClientNetwork] Login - sHearbeat received.\n";

                        delete* objIt;
                        objIt = cache.erase(objIt);
                    }
                    else if (sDisconnectResponse* disconnectResp = dynamic_cast<sDisconnectResponse*>(*objIt); disconnectResp)
                    {
                        std::cout << "[ClientNetwork] Login - sDisconnectResponse received.\n";
                        Disconnect(disconnectResp->message);
                        
                        delete* objIt;
                        objIt = cache.erase(objIt);
                    }
                    else
                    {
                        objIt++;
                    }
                }


                // Every 1 second do below
                {
                    Millis_t heartbeatPeriod(1000U);

                    if (Clock::now() >= nextHeartbeat)
                    {
                        nextHeartbeat = Clock::now() + heartbeatPeriod;

                        std::cout << "[ClientNetwork] Login - Heartbeat sent.\n";
                        session.sendCache.push_back(new sHearbeat());
                        SendOne();
                    }
                }

                SLEEP_MS(1U);
            }
            
            if (to)
            {
                loginStatus = timeoutStatus;
                isLoggingIn = false;
                isLoginFailed = true;
            }
            else
            {
               // Login OK
            }
        }
        else
        {
            isLoggingIn = false;
            isLoginFailed = true;
        }
    }

    bool Auth(std::string url);

    bool SendOne()
    {
        if (session.sendCache.size() == 0U)
            return false;

        if(!session.cacheM.try_lock())
            return false;

        bool sent = false;
        if (EasyBuffer* serializationBuffer = bufferMan->Get(); serializationBuffer)
        {
            if (MakeSerialized(serializationBuffer, session.sendCache))
            {
                if (EasyBuffer* sendBuffer = bufferMan->Get(); sendBuffer)
                {
                    if (EasyPacket::MakeCompressed(serializationBuffer, sendBuffer))
                    {
                        if (EasyPacket::MakeEncrypted({ session.sid, session.seqid_in, session.seqid_out, session.key }, sendBuffer))
                        {
                            if (uint64_t res = session.sck.send(sendBuffer->begin(), sizeof(SessionID_t) + sizeof(SequenceID_t) + IV_SIZE + sendBuffer->m_payload_size, EasyIpAddress::resolve(SERVER_IP), SERVER_PORT); res == WSAEISCONN)
                            {
                                sent = true;
                                session.seqid_out++;
                            }
                        }
                    }
                    bufferMan->Free(sendBuffer);
                }
            }
            for (EasySerializeable* obj : session.sendCache)
                delete obj;
            session.sendCache.clear();
            bufferMan->Free(serializationBuffer);
        }
        session.cacheM.unlock();
        return sent;
    }

    bool ReceiveOne()
    {
        if (!session.cacheM.try_lock())
            return false;

        bool recvd = false;
        if (EasyBuffer* buff = bufferMan->Get(); buff)
        {
            uint64_t addr = 0U;
            if (uint64_t ret = session.sck.receive(buff->begin(), buff->capacity(), buff->m_payload_size, addr); ret == WSAEISCONN)
            {
                if (buff->m_payload_size >= EasyPacket::MinimumSize())
                {
                    EasyPacket packet(buff);
                    session.sid = *packet.SessionID();
                    if (IS_SESSION(session.sid))
                    {
                        EasyPacket epck(buff);
                        CryptData crypt(session.sid, session.seqid_in, session.seqid_out, session.key);
                        if (epck.MakeDecrypted(crypt, buff))
                        {
                            if (EasyBuffer* outBuff = bufferMan->Get(); outBuff)
                            {
                                if (epck.MakeDecompressed(buff, outBuff))
                                {
                                    std::vector<EasySerializeable*> cache;
                                    if (MakeDeserialized(outBuff, cache))
                                    {
                                        session.receiveCache.insert(session.receiveCache.end(), cache.begin(), cache.end());
                                        session.seqid_in++;
                                        session.lastReceive = Clock::now();
                                        recvd = true;
                                    }
                                }
                                bufferMan->Free(outBuff);
                            }
                        }
                    }
                }
            }
            bufferMan->Free(buff);
        }
        session.cacheM.unlock();
        return recvd;
    }

    std::vector<EasySerializeable*>& GetReceiveCache()
    {
        if (session.cacheM.try_lock())
        {
            session.cacheM.unlock();
        }
        return session.receiveCache;
    }

    std::vector<EasySerializeable*>& GetSendCache()
    {
        return session.sendCache;
    }

    std::string StatusText()
    {
        return loginStatus;
    }

    void ChampionSelect(uint8_t cid)
    {
        isChampionSelect = false;
        loginStatus = "Selecting champion...";
        GetSendCache().push_back(new sChampionSelectRequest(cid));
        std::cout << "[EasyPlayground] Champion select request sent.\n";
        SendOne();
    }

    bool IsLoggingIn() const
    {
        return isLoggingIn;
    }

    bool IsLoginFailed() const
    {
        return isLoginFailed;
    }

    bool IsInGame() const
    {
        return isInGame;
    }

    bool IsChampionSelect() const
    {
        return isChampionSelect;
    }

};
