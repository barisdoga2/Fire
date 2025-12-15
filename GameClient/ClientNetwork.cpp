#include "ClientNetwork.hpp"

#include <curl/curl.h>

Millis_t heartbeatPeriod(1000U);
Millis_t timeout(3000U);
Timestamp_t nextHeartbeat;
Timestamp_t end;
std::string timeoutStatus;

ClientNetwork::ClientNetwork(EasyBufferManager* bufferMan, ClientCallback* cbk) : bufferMan(bufferMan), cbk(cbk), session()
{

}

bool ClientNetwork::Auth()
{
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

    loginStatus = "Authenticationg...";

    CURL* curl = curl_easy_init();
    if (!curl)
    {
        loginStatus = "Curl init failed!";
        isAuthing = false;
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

    curl_easy_setopt(curl, CURLOPT_URL, SERVER_URL);
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
        loginStatus = "Server not reachable!";
        isAuthing = false;
        return false;
    }
    else
    {
        curl_easy_cleanup(curl);
    }

    if (size_t index = response.find(prefix); index != std::string::npos)
    {
        loginStatus = "Waiting the game server...";

        response = response.substr(index + prefix.length());
        response = response.substr(0, response.find("</textarea>"));

        session.sid = static_cast<SessionID_t>(std::stoul(response.substr(0, response.find_first_of(':'))));

        response = response.substr(response.find_first_of(':') + 1U);
        session.uid = static_cast<uint32_t>(std::stoul(response.substr(0, response.find_first_of(':'))));

        response = response.substr(response.find_first_of(':') + 1U);
        std::string key = response;

        session.key.reserve(KEY_SIZE);
        std::memcpy(session.key.data(), key.data(), KEY_SIZE);
    }
    else
    {
        if (size_t index = response.find(prefix2); index != std::string::npos)
        {
            response = response.substr(index + prefix2.length());
            response = response.substr(0, response.find("</div>"));
            loginStatus = response;
        }
        else
        {
            loginStatus = "Error while parsing response!";
        }
        isAuthing = false;
        return false;
    }
    isAuthing = false;
    isAuth = true;

    loginStatus = "Waiting game server...";
    std::cout << "[ClientNetwork] Login - Login request sent.\n";
    session.sendCache.push_back(new sLoginRequest());

    nextHeartbeat = Clock::now();
    end = Clock::now() + timeout;
    timeoutStatus = "Login response timed out!";

    return true;
}

void ClientNetwork::Login()
{
    if (isInGame || isLoggingIn || isAuthing)
        return;

    isLoggingIn = true;
    isAuthing = true;
    isAuth = false;

    authThread = std::thread([this]() {Auth(); });
    authThread.detach();
}

void ClientNetwork::Disconnect(std::string serverMessage)
{
    isInGame = false;
    isLoggingIn = false;

    Stop();

    loginStatus = serverMessage;
    isLoginFailed = true;
}

void ClientNetwork::Stop()
{
    cbk->OnDisconnect();

    if (isInGame || isLoggingIn)
    {
        session.sendCache.push_back(new sLogoutRequest());
        SendOne();
        std::cout << "[ClientNetwork] Stop - Logout request sent!\n";
    }

    isInGame = false;
    isLoggingIn = false;
    isLoginFailed = false;
    isChampionSelect = false;
    isAuthing = false;
    isAuth = false;
    loginStatus = "";

    session.Stop();
}

void ClientNetwork::Update()
{
    for (uint8_t i = 0U; i < 16U; i++)
    {
        ReceiveOne();
        SendOne();
    }

    if (isLoggingIn)
    {
        if (isAuthing)
        {
            // nop
        }
        else
        {
            if (isAuth)
            {
                if (Clock::now() < end)
                {
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

                            std::cout << "[ClientNetwork] Login - sGameBoot received.\n";
                            isInGame = true;
                            isLoggingIn = false;
                            cbk->OnLogin();
                            break;
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
                            break; // Disconnect clears the cache already
                            //delete* objIt; // Disconnect clears the cache already
                            //objIt = cache.erase(objIt); // Disconnect clears the cache already
                        }
                        else
                        {
                            objIt++;
                        }
                    }

                    // Heartbeat
                    {
                        if (Clock::now() >= nextHeartbeat)
                        {
                            nextHeartbeat = Clock::now() + heartbeatPeriod;

                            std::cout << "[ClientNetwork] Login - Heartbeat sent.\n";
                            session.sendCache.push_back(new sHearbeat());
                            SendOne();
                        }
                    }
                }
                else
                {
                    loginStatus = timeoutStatus;
                    isLoggingIn = false;
                    isLoginFailed = true;
                    cbk->OnDisconnect();
                }
            }
            else
            {
                isLoggingIn = false;
                isLoginFailed = true;
                cbk->OnDisconnect();
            }
        }
    }
    else if (isInGame)
    {
        if (session.lastReceive + std::chrono::milliseconds(SESSION_TIMEOUT) < Clock::now())
        {
            loginStatus = "Connection timed out!";
            isInGame = false;
            isLoginFailed = true;
        }
    }
}

bool ClientNetwork::SendOne()
{
    if (session.sendCache.size() == 0U)
        return false;

    if (!session.cacheM.try_lock())
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

bool ClientNetwork::ReceiveOne()
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

std::vector<EasySerializeable*>& ClientNetwork::GetReceiveCache()
{
    if (session.cacheM.try_lock())
    {
        session.cacheM.unlock();
    }
    return session.receiveCache;
}

std::vector<EasySerializeable*>& ClientNetwork::GetSendCache()
{
    return session.sendCache;
}

std::string ClientNetwork::StatusText()
{
    return loginStatus;
}

void ClientNetwork::ChampionSelect(uint8_t cid)
{
    isChampionSelect = false;
    loginStatus = "Selecting champion...";
    GetSendCache().push_back(new sChampionSelectRequest(cid));
    std::cout << "[ClientNetwork] ChampionSelect - Champion select request sent.\n";
    SendOne();
}

bool ClientNetwork::IsLoggingIn() const
{
    return isLoggingIn;
}

bool ClientNetwork::IsLoginFailed() const
{
    return isLoginFailed;
}

bool ClientNetwork::IsInGame() const
{
    return isInGame;
}

bool ClientNetwork::IsChampionSelect() const
{
    return isChampionSelect;
}
