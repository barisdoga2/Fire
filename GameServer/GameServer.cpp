#include <filesystem>
#include <sstream>
#include <random>

#include "GameServer.hpp"
#include "ServerNet.hpp"
#include <EasySerializer.hpp>
#include <EasyUtils.hpp>

static uint64_t GetServerTimeMs()
{
    using clock = std::chrono::steady_clock;
    static const auto start = clock::now();
    return (uint64_t)std::chrono::duration_cast<std::chrono::milliseconds>(clock::now() - start).count();
}

GameServer::GameServer() : BaseServer()
{

}

GameServer::~GameServer()
{

}

bool GameServer::Start(EasyBufferManager* bm)
{
    return BaseServer::Start(bm, SERVER_PORT, this, FIRE_ENCRYPTION, FIRE_COMPRESSION);
}

void GameServer::Stop(std::string shutdownMessage)
{
    BaseServer::Stop(shutdownMessage);
}

void GameServer::ProcessReceived(double dt)
{
    ServerCache_t& send = GetSendCache();
    ServerCache_t& recv = GetReceiveCache();
    for (auto recvIt = recv.begin(); recvIt != recv.end(); )
    {
        FireSession* fSession = sessions[recvIt->first];
        if (!fSession)
        {
            for (EasySerializeable* obj : recvIt->second)
                delete obj;
            recvIt->second.clear();
            recvIt = recv.erase(recvIt);
            continue;
        }

        for (auto objIt = recvIt->second.begin(); objIt != recvIt->second.end(); )
        {
            if (sChampionSelectRequest* championSelectRequest = dynamic_cast<sChampionSelectRequest*>(*objIt); championSelectRequest)
            {
                std::cout << "[GameServer] Update - Champion select request received, response sent.\n";
                bool response = std::find(fSession->stats.champions_owned.begin(), fSession->stats.champions_owned.end(), championSelectRequest->championID) != fSession->stats.champions_owned.end();
                std::string message{};
                if (!response)
                    message = "Buy this champion first!";
                else
                    message = "Champion selected.";
                send[recvIt->first].push_back(new sChampionSelectResponse(response, message));

                if (response)
                {
                    send[recvIt->first].push_back(new sGameBoot(sessions));
                    std::cout << "[GameServer] Update - Game boot sent.\n";
                }

                fSession->recv = Clock::now();
                delete* objIt;
                objIt = recvIt->second.erase(objIt);
            }
            else if (sPlayerInput* playerInput = dynamic_cast<sPlayerInput*>(*objIt); playerInput)
            {
                //std::cout << "[GameServer] Update - sPlayerInput received.\n";

                fSession->inputQueue.push_back(*playerInput);
                while (fSession->inputQueue.size() > 64U)
                    fSession->inputQueue.pop_front();

                fSession->recv = Clock::now();
                delete* objIt;
                objIt = recvIt->second.erase(objIt);
            }
            else if (sLogoutRequest* logoutRequest = dynamic_cast<sLogoutRequest*>(*objIt); logoutRequest)
            {
                std::cout << "[GameServer] Update - Logout request received.\n";
                fSession->logoutRequested = true;

                fSession->recv = Clock::now();
                delete* objIt;
                objIt = recvIt->second.erase(objIt);
            }
            else if (sHearbeat* heartbeat = dynamic_cast<sHearbeat*>(*objIt); heartbeat)
            {
                send[fSession->sid].push_back(new sHearbeat());
                std::cout << "[GameServer] Update - Heartbeat received and sent.\n";

                fSession->recv = Clock::now();
                delete* objIt;
                objIt = recvIt->second.erase(objIt);
            }
            else if (sChatMessage* chatMessage = dynamic_cast<sChatMessage*>(*objIt); chatMessage)
            {
                for (auto& [sid, fs] : sessions)
                    if (fs)
                        send[fs->sid].push_back(new sChatMessage(fSession->username, chatMessage->message, Clock::now().time_since_epoch().count()));

                std::cout << "[GameServer] Update - Chat message received and distributed.\n";

                fSession->recv = Clock::now();
                delete* objIt;
                objIt = recvIt->second.erase(objIt);
            }
            else
            {
                objIt++;
            }
        }

        if (recvIt->second.size() == 0U)
            recvIt = recv.erase(recvIt);
        else
            recvIt++;
    }
}

void GameServer::Tick(double dt)
{
    ServerCache_t& send = GetSendCache();

    // ---- FIXED TICK ----
    static double acc = 0.0;
    constexpr double SERVER_DT = 0.1;// 0.05; // 50ms (20Hz) — istersen 0.1 yap

    acc += dt;
    while (acc >= SERVER_DT)
    {
        acc -= SERVER_DT;

        const float speed = 5.0f;
        const float dts = (float)SERVER_DT;

        // 1) input uygula (otorite)
        //std::cout << "[GameServer] Tick - Inputs applied.\n";
        for (auto& [sid, fs] : sessions)
        {
            if (!fs) continue;

            // bu tick’te en fazla 1 input işle (bare minimum)
            if (!fs->inputQueue.empty())
            {
                sPlayerInput in = fs->inputQueue.front();
                fs->inputQueue.pop_front();

                // eski/duplicate input koruması
                if (in.inputSeq > fs->lastInputSeq)
                {
                    fs->lastInputSeq = in.inputSeq;

                    glm::vec3 dir = in.moveDir;
                    if (glm::length(dir) > 0.f) dir = glm::normalize(dir);

                    fs->velocity = dir * speed;
                    fs->position += fs->velocity * dts;
                }
            }
            else
            {
                fs->velocity = glm::vec3(0.f);
            }
        }
        //std::cout << "[GameServer] Tick - sPlayerState broadcasted.\n";

        // 2) state broadcast (her tick)
        const uint64_t ts = GetServerTimeMs();
        for (auto& [sid, fs] : sessions)
        {
            if (!fs) continue;

            // Her client'a, her player'ın state'i (en basit worldstate gibi)
            for (auto& [sid2, fs2] : sessions)
            {
                if (!fs2) continue;

                send[sid].push_back(new sPlayerState(
                    fs2->uid,
                    fs2->position,
                    fs2->velocity,
                    fs2->lastInputSeq,
                    ts
                ));
            }
        }
    }
}

void GameServer::Update(double dt)
{
    if (!IsRunning())
        return;

    BaseServer::Update(dt);

    ProcessReceived(dt);

    Tick(dt);

    // Destroy Sessions. Timeout and Logout
    {
        std::vector<std::pair<SessionID_t, std::string>> sessionsToDestroy{};
        for (auto& [sid, fSession] : sessions)
        {
            if (fSession->recv + std::chrono::milliseconds(SESSION_TIMEOUT) < Clock::now())
            {
                std::cout << "[GameServer] Update - Session timed out.\n";
                sessionsToDestroy.push_back({ sid, "Session timed out." });
            }
            else if (fSession->logoutRequested)
            {
                sessionsToDestroy.push_back({ sid, "" });
            }
        }
        for (const std::pair<SessionID_t, std::string>& s2d : sessionsToDestroy)
        {
            DestroySession(s2d.first, s2d.second);
        }
    }
}

void GameServer::BroadcastMessage(std::string broadcastMessage)
{
    ServerCache_t& send = GetSendCache();
    for (auto& [sid, fs] : sessions)
        send[sid].push_back(new sBroadcastMessage(broadcastMessage));
}

bool GameServer::OnServerStart()
{
    std::cout << "[GameServer] OnServerStart - Starting...\n";

    if (!sqlite.Init(GetRelPath("res/db/fire.db")))
    {
        std::cout << "[GameServer] OnServerStart - Failed to start. Failed to open database!\n";
        return false;
    }
    else
    {
        std::cout << "[GameServer] OnServerStart - Opened databased.\n";
    }
    std::cout << "[GameServer] OnServerStart - Started.\n";

    return true;
}

void GameServer::OnServerStop(std::string shutdownMessage)
{
    std::cout << "[GameServer] OnServerStart - Stopping...\n";

    ServerCache_t& send = GetSendCache();
    for (auto& [sid, fs] : sessions)
        send[sid].push_back(new sDisconnectResponse(shutdownMessage));
    GameServer::Update(0.0); // Send
    for (auto& [sid, fs] : sessions)
        delete fs;
    sessions.clear();

    sqlite.Close();
    std::cout << "[GameServer] OnServerStart - Stopped.\n";
}

bool GameServer::OnSessionCreate(const SessionBase& base, EasyBuffer* buffer)
{
    std::cout << "[GameServer] OnSessionCreate - Creating session.\n";

    std::string username;
    UserStats stats;
    (*base.key).reserve(KEY_SIZE);

    bool status = true;
    if (status)
    {
        if (auto db_sessions = sqlite.Query("SELECT id, user_id, session_key, valid_until FROM sessions WHERE id='" + std::to_string(*base.sid) + "' LIMIT 1;"); !db_sessions.empty())
        {
            const auto& db_session = db_sessions[0];
            stats.uid = std::stoi(db_session[1]);
            std::string keyStr = db_session[2];
            std::string expiryStr = db_session[3];
            memcpy(base.key->data(), keyStr.data(), KEY_SIZE);
            std::tm tm_parsed{};
            (std::istringstream{ db_session[1] }) >> std::get_time(&tm_parsed, "%Y-%m-%d %H:%M:%S");
            std::time_t parsed_time = std::mktime(&tm_parsed);
            *base.keyExpr = std::chrono::system_clock::from_time_t(parsed_time);

            if (std::vector<EasySerializeable*> res = Process({ *base.sid, 0U, 0U, *base.key}, buffer); res.size() != 0U)
            {
                status = false;
                for (auto it = res.begin() ; it != res.end() ; )
                {
                    if (sLoginRequest* loginRequest = dynamic_cast<sLoginRequest*>(*it); loginRequest)
                    {
                        std::cout << "[GameServer] OnSessionCreate - Login request received.\n";
                        status = true;
                    }
                    delete* it;
                    it = res.erase(it);
                }
            }
            else
            {
                std::cout << "[GameServer] OnSessionCreate - Expected login request but something else received.\n";
                status = false;
            }
        }
        else
        {
            std::cout << "[GameServer] OnSessionCreate - SELECT from 'sessions' failed.\n";
            status = false;
        }
    }

    if (status)
    {
        if (auto db_users = sqlite.Query("SELECT id, username FROM users WHERE id='" + std::to_string(stats.uid) + "' LIMIT 1;"); !db_users.empty())
        {
            const auto& db_user = db_users[0];
            username = db_user[1];
        }
        else
        {
            std::cout << "[GameServer] OnSessionCreate - SELECT from 'users' failed.\n";
            status = false;
        }
    }

    if (status)
    {
        if (auto db_stats = sqlite.Query("SELECT id, user_id, gametime, golds, diamonds, tutorial_done, champions_owned FROM user_stats WHERE user_id='" + std::to_string(stats.uid) + "' LIMIT 1;"); !db_stats.empty())
        {
            const auto& db_stat = db_stats[0];

            stats.gametime = std::stoi(db_stat[2]);
            stats.golds = std::stoi(db_stat[3]);
            stats.diamonds = std::stoi(db_stat[4]);
            stats.tutorial_done = std::stoi(db_stat[5]) != 0;

            std::string ownedStr = db_stat[6];
            size_t pos = 0;
            while ((pos = ownedStr.find(',')) != std::string::npos)
            {
                stats.champions_owned.push_back(std::stoi(ownedStr.substr(0, pos)));
                ownedStr.erase(0, pos + 1);
            }
            stats.champions_owned.push_back(std::stoi(ownedStr));
        }
        else
        {
            std::cout << "[GameServer] OnSessionCreate - SELECT from 'user_stats' failed.\n";
            status = false;
        }
    }

    std::cout << "[GameServer] OnSessionCreate - Login response sent.\n";
    GetSendCache()[*base.sid].push_back(new sLoginResponse(status, "GameServer"));

    if (status)
    {
        std::cout << "[GameServer] OnSessionCreate - New Session Created.\n";

        auto& send = GetSendCache();
        for (auto& [sid, fs] : sessions)
            if (fs)
                send[fs->sid].push_back(new sChatMessage("[Server]", username + " logged in!", Clock::now().time_since_epoch().count()));

        FireSession* fSession = new FireSession(username, *base.sid, stats.uid, *base.addr, stats, *base.recv);
        sessions[*base.sid] = fSession;

        std::cout << "[GameServer] OnSessionCreate - Player boot info sent.\n";
        send[*base.sid].push_back(new sPlayerBootInfo(fSession->uid, fSession->stats.diamonds, fSession->stats.golds, fSession->stats.gametime, fSession->stats.tutorial_done, fSession->stats.champions_owned));
    }
    else
    {
        std::cout << "[GameServer] OnSessionCreate - Session Create Failed!\n";
    }

    return status;
}

void GameServer::OnSessionDestroy(const SessionBase& base, std::string disconnectMessage)
{
    std::cout << "[GameServer] OnSessionDestroy - Session Destroyed.\n";

    SessionID_t sid = *base.sid;

    if (disconnectMessage.length() > 0U)
    {
        sDisconnectResponse* disconnect = new sDisconnectResponse(disconnectMessage);
        Send(sid, { disconnect });
        delete disconnect;
    }

    auto& send = GetSendCache();
    for (auto& [sid, fs] : sessions)
        if (fs)
            send[fs->sid].push_back(new sChatMessage("[Server]", sessions[sid]->username + " logged out!", Clock::now().time_since_epoch().count()));

    delete sessions[sid];
    sessions[sid] = nullptr;
    sessions.erase(sessions.find(sid));
}

bool GameServer::OnSessionKeyExpiry(const SessionBase& base)
{
    std::cout << "[GameServer] OnSessionKeyExpiry - Session Key Expired.\n";
    
    // ...

    return false;
}

bool GameServer::OnSessionReconnect(const SessionBase& base, const SessionBase& reconnectingBase)
{
    std::cout << "[GameServer] OnSessionReconnect - Session reconnected.\n";

    SessionID_t sid = *base.sid;

    Addr_t addr = *reconnectingBase.addr;

    Key_t key;
    key.insert(key.end(), base.key->begin(), base.key->end());

    BaseServer::DestroySession(sid, "Reconnected from another client!");

    sDisconnectResponse* disconnect = new sDisconnectResponse("Account was already in game, disconnected, try again!");
    Send(sid, { disconnect }, addr, 0U, 0U, key);
    delete disconnect;

    return false;
}
