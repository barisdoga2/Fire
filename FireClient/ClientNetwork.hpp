#pragma once

#include <string>
#include <iostream>
#include <vector>
#include <curl/curl.h>
#include <EasySocket.hpp>
#include "FireNet.hpp"
#include "Config.hpp"
#include "../FireServer/ServerNet.hpp"
#include <EasyIpAddress.hpp>

class ClientSession {
public:
    std::string username, password;
    SessionID_t sid;
    Addr_t addr;
    Key_t key;
    Key_Expt_t key_expiry;
    SequenceID_t seqid_in, seqid_out;
    Timestamp_t lastReceive;

    UserID_t uid;
    EasySocket sck;
    std::mutex cacheM;
    std::vector<EasySerializeable*> receiveCache;
    std::vector<EasySerializeable*> sendCache;
    UserStats stats;

};

class ClientNetwork {
    EasyBufferManager* bufferMan;

public:
    bool isAuth{};
    bool isLoggedIn{};
    bool isInGame{};
    bool isChampionSelect{};
    bool isChampionSelected{};
    bool isBoot{};
    bool isLoginFailed{};

    ClientSession session;

    ClientNetwork(EasyBufferManager* bufferMan) : bufferMan(bufferMan), session()
    {

    }

    void Update()
    {
        ReceiveOne();
        SendOne();
    }

    void Login(std::string url, std::string& loginStatusText, bool rememberMe, std::atomic<bool>& isLoginThreadRunning)
    {
        if (isInGame || isLoggedIn || isInGame || isChampionSelect || isChampionSelected || isBoot)
            return;

        isLoginThreadRunning.store(true);

        isLoginFailed = false;

        isAuth = Auth(SERVER_URL, loginStatusText, rememberMe);
        if (isAuth)
        {
            loginStatusText = "Waiting game server...";
            std::cout << "[ClientNetwork] Login - Login request sent.\n";
            session.cacheM.lock();
            session.sendCache.push_back(new sLoginRequest());
            session.cacheM.unlock();
        }

        bool to{};
        Millis_t timeout = Millis_t(3000U);
        Timestamp_t end = Clock::now() + timeout;
        std::string timeoutStatus = "Login response timed out!";
        while (!to && isAuth && !isInGame && !isLoginFailed && isLoginThreadRunning.load())
        {
            to = Clock::now() > end;
            if (!session.cacheM.try_lock())
                continue;

            std::vector<EasySerializeable*>& cache = GetReceiveCache();
            for (std::vector<EasySerializeable*>::iterator objIt = cache.begin() ; objIt != cache.end() && isAuth ; )
            {
                if (sLoginResponse* loginResp = dynamic_cast<sLoginResponse*>(*objIt); loginResp && !isLoggedIn)
                {
                    end = Clock::now() + timeout;
                    timeoutStatus = "Player boot info timed out!";

                    std::cout << "[ClientNetwork] Login - sLoginResponse received.\n";
                    loginStatusText = loginResp->message;
                    isAuth = loginResp->response;
                    isLoggedIn = loginResp->response;

                    delete* objIt;
                    objIt = cache.erase(objIt);
                }
                else if (sPlayerBootInfo* bootInfo = dynamic_cast<sPlayerBootInfo*>(*objIt); bootInfo && !isBoot && isLoggedIn && !isChampionSelected && !isChampionSelect)
                {
                    end = Clock::now() + timeout;
                    timeoutStatus = "Champion select response timed out!";

                    std::cout << "[ClientNetwork] Login - sPlayerBootInfo received.\n";
                    loginStatusText = "PlayerBootInfo received.";
                    isBoot = true;

                    session.stats.uid = bootInfo->userID;
                    session.stats.gametime = bootInfo->gametime;
                    session.stats.golds = bootInfo->golds;
                    session.stats.diamonds = bootInfo->diamonds;
                    session.stats.tutorial_done = bootInfo->tutorialDone;
                    session.stats.champions_owned = bootInfo->championsOwned;

                    delete* objIt;
                    objIt = cache.erase(objIt);
                }
                else if (sChampionSelectResponse* championSelectResp = dynamic_cast<sChampionSelectResponse*>(*objIt); championSelectResp && isBoot && isChampionSelected && !isChampionSelect)
                {
                    end = Clock::now() + timeout;
                    timeoutStatus = "Heartbeat timed out!";

                    std::cout << "[ClientNetwork] Login - sChampionSelectResponse received.\n";
                    loginStatusText = championSelectResp->message;
                    isChampionSelect = championSelectResp->response;

                    if(isChampionSelect)
                        session.sendCache.push_back(new sHearbeat());

                    delete* objIt;
                    objIt = cache.erase(objIt);
                }
                else if (sHearbeat* heartbeat = dynamic_cast<sHearbeat*>(*objIt); heartbeat && isChampionSelect)
                {
                    end = Clock::now() + timeout;
                    timeoutStatus = "Heartbeat timed out!";

                    std::cout << "[ClientNetwork] Login - sHearbeat received.\n";
                    isInGame = true;

                    delete* objIt;
                    objIt = cache.erase(objIt);
                }
                else if (sDisconnectResponse* disconnectResp = dynamic_cast<sDisconnectResponse*>(*objIt); disconnectResp)
                {
                    std::cout << "[ClientNetwork] Login - sDisconnectResponse received.\n";
                    loginStatusText = disconnectResp->message;
                    isLoggedIn = false;
                    isInGame = false;
                    isChampionSelect = false;
                    isChampionSelected = false;
                    isAuth = false;
                    isBoot = false;
                    isLoginFailed = false;

                    delete* objIt;
                    objIt = cache.erase(objIt);
                }
                else
                {
                    objIt++;
                }
            }
            session.cacheM.unlock();

            SLEEP_MS(1U);
            
        }


        if (to)
        {
            loginStatusText = timeoutStatus;
            isLoginFailed = true;
        }
        isLoggedIn = isInGame;
        isChampionSelect = false;
        isChampionSelected = isInGame;
        isAuth = isInGame;
        isBoot = isInGame;
        isLoginFailed = isInGame ? false : isLoginFailed;
        isLoginThreadRunning.store(isInGame);
    }

    bool Auth(std::string url, std::string& statusMessage, bool rememberMe)
    {
        if (isAuth || isLoggedIn || isChampionSelect || isInGame || isBoot)
            return false;

        static const std::string prefix = "<textarea name=\"jwt\" readonly>";
        static const std::string prefix2 = "<div class=\"msg\">";

        session.sck = {};
        session.sid = 0U;
        session.uid = 0U;
        session.addr = 0U;
        session.key = Key_t(KEY_SIZE);
        session.key_expiry = {};
        session.seqid_in = 0U;
        session.seqid_out = 0U;
        session.lastReceive = {};
        session.receiveCache = {};
        session.sendCache = {};
        session.stats = {};

        statusMessage = "Authenticationg...";

        CURL* curl = curl_easy_init();
        if (!curl)
        {
            statusMessage = "Curl init failed!";
            return false;
        }

        std::string response;
        std::string postData = std::string("username=").append(session.username.c_str()).append("&password=").append(session.password.c_str()).append("&login=1");
        auto writeLambda = [](char* ptr, size_t size, size_t nmemb, void* userdata) -> size_t
            {
                auto* out = static_cast<std::string*>(userdata);
                out->append(ptr, size * nmemb);
                return size * nmemb;
            };

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, +writeLambda);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, &response, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK)
        {
            std::string err = curl_easy_strerror(res);
            curl_easy_cleanup(curl);
            statusMessage = "Server not reachable!";
            return false;
        }
        else
        {
            curl_easy_cleanup(curl);
        }

        if (size_t index = response.find(prefix); index != std::string::npos)
        {
            statusMessage = "Waiting the game server...";

            response = response.substr(index + prefix.length());
            response = response.substr(0, response.find("</textarea>"));

            session.sid = static_cast<SessionID_t>(std::stoul(response.substr(0, response.find_first_of(':'))));

            response = response.substr(response.find_first_of(':') + 1U);
            session.uid = static_cast<uint32_t>(std::stoul(response.substr(0, response.find_first_of(':'))));

            response = response.substr(response.find_first_of(':') + 1U);
            std::string key = response;

            session.key.reserve(KEY_SIZE);
            std::memcpy(session.key.data(), key.data(), KEY_SIZE);

            if (rememberMe)
                SaveConfig(true, session.username, session.password);
            else
                SaveConfig(false, "", "");
        }
        else
        {
            if (size_t index = response.find(prefix2); index != std::string::npos)
            {
                response = response.substr(index + prefix2.length());
                response = response.substr(0, response.find("</div>"));
                statusMessage = response;
            }
            else
            {
                statusMessage = "Error while parsing response!";
            }
            return false;
        }
        isAuth = true;
        return true;
    }

    void Stop()
    {
        isAuth = false;

        isLoggedIn = false;
        isLoginFailed = false;

        isBoot = false;

        isChampionSelect = false;
        isChampionSelected = false;
        
        isInGame = false;

        //std::vector<EasySerializeable*> logout = { new sLogoutRequest() };
        //if ((loggedIn || !loginFailed || champSelect || loginInProgress) && client.client.crypt)
        //{
        //    client.ClientSend(*client.client.crypt, logout);
        //    std::cout << "Logout request sent!\n";
        //}
        //loggedIn = false;
        //loginFailed = true;
        //loginInProgress = false;
        //loginStatusWindow = false;
        //champSelect = false;
        //// reset crypt, username, session, etc.
        //if (client.client.crypt) { delete client.client.crypt; client.client.crypt = nullptr; }
        //sessionID = 0;
        //username = "";
        //stats = {}; // reset
    }

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
            uint64_t addr;
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

};
