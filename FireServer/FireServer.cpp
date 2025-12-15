#include <filesystem>
#include <sstream>
#include <random>

#include "FireServer.hpp"
#include "ServerNet.hpp"
#include <EasySerializer.hpp>
#include <EasyUtils.hpp>



FireServer::FireServer(EasyBufferManager* bm, unsigned short port) : BaseServer(bm, port, this, FIRE_ENCRYPTION, FIRE_COMPRESSION)
{

}

FireServer::~FireServer()
{

}
void FireServer::Update(double dt)
{
    if (!IsRunning())
        return;

    BaseServer::Update(dt);

    static std::unordered_map<SessionID_t, std::vector<sMoveInput*>> playerInputs;

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
                std::cout << "[FireServer] Update - Champion select request received, response sent.\n";
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
                    std::cout << "[FireServer] Update - Game boot sent.\n";
                }

                fSession->recv = Clock::now();
                delete* objIt;
                objIt = recvIt->second.erase(objIt);
            }
            else if (sLogoutRequest* logoutRequest = dynamic_cast<sLogoutRequest*>(*objIt); logoutRequest)
            {
                std::cout << "[FireServer] Update - Logout request received.\n";
                fSession->logoutRequested = true;

                fSession->recv = Clock::now();
                delete* objIt;
                objIt = recvIt->second.erase(objIt);
            }
            else if (sHearbeat* heartbeat = dynamic_cast<sHearbeat*>(*objIt); heartbeat)
            {
                send[fSession->sid].push_back(new sHearbeat());
                std::cout << "[FireServer] Update - Heartbeat received and sent.\n";

                fSession->recv = Clock::now();
                delete* objIt;
                objIt = recvIt->second.erase(objIt);
            }
            else if (sChatMessage* chatMessage = dynamic_cast<sChatMessage*>(*objIt); chatMessage)
            {
                for(auto& [sid, fs] : sessions)
                    if(fs)
                        send[fs->sid].push_back(new sChatMessage(chatMessage->message, fSession->username, Clock::now().time_since_epoch().count()));

                std::cout << "[FireServer] Update - Chat message received and distributed.\n";

                fSession->recv = Clock::now();
                delete* objIt;
                objIt = recvIt->second.erase(objIt);
            }
            else if (sMoveInput* moveInput = dynamic_cast<sMoveInput*>(*objIt); moveInput)
            {
                std::cout << "[FireServer] Update - Player input received.\n";
                
                playerInputs[fSession->sid].push_back(moveInput);

                fSession->recv = Clock::now();
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

    static Timestamp_t nextInputProcessing = Clock::now();
    if (nextInputProcessing <= Clock::now() )
    {
        nextInputProcessing = Clock::now() + std::chrono::milliseconds(100U);

        for (auto& [sid, inputs] : playerInputs)
        {
            for (sMoveInput* input : inputs)
            {
                sessions[sid]->position.x += 1.0f;

                delete input;
            }
        }
        playerInputs.clear();
    }

    static Timestamp_t nextStateProcessing = Clock::now();
    if (nextStateProcessing <= Clock::now())
    {
        nextStateProcessing = Clock::now() + std::chrono::milliseconds(100U);

        for (auto& [sid, session] : sessions)
            send[sid].push_back(new sWorldState(sessions));
    }

    std::vector<SessionID_t> sessionsToLogout;
    std::vector<SessionID_t> sessionsToTimeout;
    for (auto& [sid, fSession] : sessions)
    {   
        if (fSession->recv + std::chrono::milliseconds(SESSION_TIMEOUT) < Clock::now())
            sessionsToTimeout.push_back(sid);
        else if(fSession->logoutRequested)
            sessionsToLogout.push_back(sid);
    }
    for (SessionID_t& sid : sessionsToLogout)
    {
        DestroySession(sid);
    }
    for (SessionID_t& sid : sessionsToTimeout)
    {
        std::cout << "[FireServer] Update - Session timed out.\n";
        DestroySession(sid, "Session timed out.");
    }
}

void FireServer::BroadcastMessage(std::string broadcastMessage)
{
    ServerCache_t& send = GetSendCache();
    for (auto& [sid, fs] : sessions)
        send[sid].push_back(new sBroadcastMessage(broadcastMessage));
}

bool FireServer::OnServerStart()
{
    std::cout << "[FireServer] OnServerStart - Starting...\n";

    if (!sqlite.Init(GetRelPath("res/db/fire.db")))
    {
        std::cout << "[FireServer] OnServerStart - Failed to start. Failed to open database!\n";
        return false;
    }
    else
    {
        std::cout << "[FireServer] OnServerStart - Opened databased.\n";
    }
    std::cout << "[FireServer] OnServerStart - Started.\n";

    return true;
}

void FireServer::OnServerStop(std::string shutdownMessage)
{
    std::cout << "[FireServer] OnServerStart - Stopping...\n";

    ServerCache_t& send = GetSendCache();
    for (auto& [sid, fs] : sessions)
        send[sid].push_back(new sDisconnectResponse(shutdownMessage));
    FireServer::Update(0.0); // Send

    sqlite.Close();
    std::cout << "[FireServer] OnServerStart - Stopped.\n";
}

bool FireServer::OnSessionCreate(const SessionBase& base, EasyBuffer* buffer)
{
    std::cout << "[FireServer] OnSessionCreate - Creating session.\n";

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
                        std::cout << "[FireServer] OnSessionCreate - Login request received.\n";
                        status = true;
                    }
                    delete* it;
                    it = res.erase(it);
                }
            }
            else
            {
                std::cout << "[FireServer] OnSessionCreate - Expected login request but something else received.\n";
                status = false;
            }
        }
        else
        {
            std::cout << "[FireServer] OnSessionCreate - SELECT from 'sessions' failed.\n";
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
            std::cout << "[FireServer] OnSessionCreate - SELECT from 'users' failed.\n";
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
            std::cout << "[FireServer] OnSessionCreate - SELECT from 'user_stats' failed.\n";
            status = false;
        }
    }

    std::cout << "[FireServer] OnSessionCreate - Login response sent.\n";
    GetSendCache()[*base.sid].push_back(new sLoginResponse(status, "FireServer"));

    if (status)
    {
        std::cout << "[FireServer] OnSessionCreate - New Session Created.\n";

        auto& send = GetSendCache();
        for (auto& [sid, fs] : sessions)
            if (fs)
                send[fs->sid].push_back(new sChatMessage("[Server]", username + " logged in!", Clock::now().time_since_epoch().count()));

        FireSession* fSession = new FireSession(username, *base.sid, stats.uid, *base.addr, stats, *base.recv);
        sessions[*base.sid] = fSession;

        std::cout << "[FireServer] OnSessionCreate - Player boot info sent.\n";
        send[*base.sid].push_back(new sPlayerBootInfo(fSession->uid, fSession->stats.diamonds, fSession->stats.golds, fSession->stats.gametime, fSession->stats.tutorial_done, fSession->stats.champions_owned));
    }
    else
    {
        std::cout << "[FireServer] OnSessionCreate - Session Create Failed!\n";
    }

    return status;
}

void FireServer::OnSessionDestroy(const SessionBase& base, std::string disconnectMessage)
{
    std::cout << "[FireServer] OnSessionDestroy - Session Destroyed.\n";

    SessionID_t sid = *base.sid;

    if(disconnectMessage.length() > 0U)
        Send(sid, { new sDisconnectResponse(disconnectMessage) });

    auto& send = GetSendCache();
    for (auto& [sid, fs] : sessions)
        if (fs)
            send[fs->sid].push_back(new sChatMessage("[Server]", sessions[sid]->username + " logged out!", Clock::now().time_since_epoch().count()));

    delete sessions[sid];
    sessions[sid] = nullptr;
    sessions.erase(sessions.find(sid));
}

bool FireServer::OnSessionKeyExpiry(const SessionBase& base)
{
    std::cout << "[FireServer] OnSessionKeyExpiry - Session Key Expired.\n";
    
    // ...

    return false;
}

bool FireServer::OnSessionReconnect(const SessionBase& base, const SessionBase& reconnectingBase)
{
    std::cout << "[FireServer] OnSessionReconnect - Session reconnected.\n";

    SessionID_t sid = *base.sid;

    Addr_t addr = *reconnectingBase.addr;

    Key_t key;
    key.insert(key.end(), base.key->begin(), base.key->end());

    BaseServer::DestroySession(sid, "Reconnected from another client!");

    Send(sid, { new sDisconnectResponse("Account was already in game, disconnected, try again!") }, addr, 0U, 0U, key);

    return false;
}
