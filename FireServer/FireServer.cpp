#include <filesystem>
#include <sstream>
#include <random>
#include <Windows.h>

#include "FireServer.hpp"
#include "ServerNet.hpp"
#include <EasySerializer.hpp>

inline std::string GetPath(std::string append)
{
    static std::string root;
    static bool i = true;
    if (i)
    {
        i = false;
        char buffer[128];
        GetModuleFileNameA(NULL, buffer, 128);
        std::string fullPath(buffer);
        for (auto& c : fullPath)
            if (c == '\\')
                c = '/';
        size_t pos = fullPath.find_last_of("\\/");
        root = fullPath.substr(0, pos) + '/';

        bool resFolder = std::filesystem::exists(root + "res") && std::filesystem::is_directory(root + "res");
        if (!resFolder)
        {
            root = root + "../";
            resFolder = std::filesystem::exists(root + "res") && std::filesystem::is_directory(root + "res");
        }

        if (!resFolder)
        {
            root = root + "../";
            resFolder = std::filesystem::exists(root + "res") && std::filesystem::is_directory(root + "res");
        }

        if (!resFolder)
        {
            root = root + "../";
            resFolder = std::filesystem::exists(root + "res") && std::filesystem::is_directory(root + "res");
        }
    }

    return root + append;
}


FireServer::FireSession::FireSession(std::string username, SessionID_t sid, UserID_t uid, Addr_t addr, UserStats stats, Timestamp_t recv) : username(username), sid(sid), uid(uid), addr(addr), stats(stats), recv(recv), logoutRequested(false)
{

}

FireServer::FireServer(EasyBufferManager* bm, unsigned short port) : BaseServer(bm, port, this, FIRE_ENCRYPTION, FIRE_COMPRESSION)
{

}

FireServer::~FireServer()
{

}

void FireServer::Update()
{
    BaseServer::Update();

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
                std::cout << "[FireServer] Update - Champion select request received.\n";
                bool response = std::find(fSession->stats.champions_owned.begin(), fSession->stats.champions_owned.end(), championSelectRequest->championID) != fSession->stats.champions_owned.end();
                std::string message{};
                if (!response)
                    message = "Buy this champion first!";
                else
                    message = "Champion selected.";
                send[recvIt->first].push_back(new sChampionSelectResponse(response, message));
                std::cout << "[FireServer] Update - Champion select response sent.\n";
                delete* objIt;
                objIt = recvIt->second.erase(objIt);
            }
            else if (sLogoutRequest* logoutRequest = dynamic_cast<sLogoutRequest*>(*objIt); logoutRequest)
            {
                std::cout << "[FireServer] Update - Logout request received.\n";
                fSession->logoutRequested = true;
                delete* objIt;
                objIt = recvIt->second.erase(objIt);
            }
            else if (sHearbeat* heartbeat = dynamic_cast<sHearbeat*>(*objIt); heartbeat)
            {
                GetSendCache()[fSession->sid].push_back(new sHearbeat());
                std::cout << "[FireServer] Update - Heartbeat received and sent!\n";
                delete* objIt;
                objIt = recvIt->second.erase(objIt);
            }
            else if (sLogoutRequest* logoutRequest = dynamic_cast<sLogoutRequest*>(*objIt); logoutRequest)
            {
                std::cout << "[FireServer] Update - Logout request received!\n";
                fSession->logoutRequested = true;
                delete* objIt;
                objIt = recvIt->second.erase(objIt);
            }
            else if (sChatMessage* chatMessage = dynamic_cast<sChatMessage*>(*objIt); chatMessage)
            {
                //messages.push_back(new sChatMessage(chatMessage->message, fSession->username, Clock::now().time_since_epoch().count()));
                std::cout << "[FireServer] Update - Chat message received!\n";
                delete* objIt;
                objIt = recvIt->second.erase(objIt);
            }
            else if (sPlayerMovement* playerMovement = dynamic_cast<sPlayerMovement*>(*objIt); playerMovement)
            {
                std::cout << "[FireServer] Update - Player movement message received!\n";
                if (fSession->uid == playerMovement->userID)
                {
                    //fSession->position = playerMovement->position;
                    //fSession->rotation = playerMovement->rotation;
                    //fSession->direction = playerMovement->direction;
                    //fSession->moveTimestamp = playerMovement->timestamp;
                }
                delete* objIt;
                objIt = recvIt->second.erase(objIt);
            }
            else
            {
                objIt++;
            }
        }
        recvIt++;
    }
}

bool FireServer::OnServerStart()
{
    std::cout << "[FireServer] Starting...\n";

    if (!sqlite.Init(GetPath("res/db/fire.db")))
    {
        std::cout << "[FireServer] Failed to start. Failed to open database!\n";
        return false;
    }
    else
    {
        std::cout << "[FireServer] Opened databased.\n";
    }
    std::cout << "[FireServer] Started.\n";

    return true;
}

void FireServer::OnServerStop()
{
    std::cout << "[FireServer] Stopping...\n";
    sqlite.Close();
    std::cout << "[FireServer] Stopped.\n";
}

bool FireServer::OnSessionCreate(const SessionBase& base)
{
    std::cout << "[FireServer] - OnSessionCreate.\n";

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

        FireSession* fSession = new FireSession(username, *base.sid, stats.uid, *base.addr, stats, *base.recv);
        sessions[*base.sid] = fSession;

        std::cout << "[FireServer] OnSessionCreate - Player boot info sent.\n";
        GetSendCache()[*base.sid].push_back(new sPlayerBootInfo(fSession->uid, fSession->stats.diamonds, fSession->stats.golds, fSession->stats.gametime, fSession->stats.tutorial_done, fSession->stats.champions_owned));
    }
    else
    {
        std::cout << "[FireServer] OnSessionCreate - Session Create Failed!\n";
    }

    return status;
}

void FireServer::OnSessionDestroy(const SessionBase& base)
{
    std::cout << "[FireServer] OnSessionDestroy - Session Destroyed.\n";

    delete sessions[*base.sid];
    sessions[*base.sid] = nullptr;
}

bool FireServer::OnSessionKeyExpiry(const SessionBase& base)
{
    std::cout << "[FireServer] OnSessionKeyExpiry - Session Key Expired.\n";

    delete sessions[*base.sid];
    sessions[*base.sid] = nullptr;
    return false;



    FireSession* fSession = sessions[*base.sid];
    if (!fSession)
        return false;

    std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<int> dist(0, 255);
    for (auto& b : *base.key)
        b = static_cast<uint8_t>(dist(gen));

    auto now = std::chrono::system_clock::now();
    auto expiry_tp = now + std::chrono::hours(1);
    std::time_t expiry_time_t = std::chrono::system_clock::to_time_t(expiry_tp);
    std::tm tm_out{};
#ifdef _WIN32
    localtime_s(&tm_out, &expiry_time_t);
#else
    localtime_r(&expiry_time_t, &tm_out);
#endif
    std::string expiryStr = (std::ostringstream{} << std::put_time(&tm_out, "%Y-%m-%d %H:%M:%S")).str();
    std::string keyStr(reinterpret_cast<const char*>(base.key->data()), KEY_SIZE);
    std::string updateSql = "UPDATE sessions SET session_key='" + keyStr + "', valid_until='" + expiryStr + "' WHERE user_id=" + std::to_string(fSession->uid) + ";";
    if (!sqlite.Execute(updateSql))
        return false;

    auto db_sessions = sqlite.Query("SELECT session_key, valid_until FROM sessions WHERE user_id=" + std::to_string(fSession->uid) + " LIMIT 1;");
    if (db_sessions.empty())
        return false;

    const auto& db_session = db_sessions[0];

    std::tm tm_parsed{};
    (std::istringstream{ db_session[1] }) >> std::get_time(&tm_parsed, "%Y-%m-%d %H:%M:%S");
    std::time_t parsed_time = std::mktime(&tm_parsed);
    auto parsed_tp = std::chrono::system_clock::from_time_t(parsed_time);

    *base.keyExpr = parsed_tp;

    return true;
}

bool FireServer::OnSessionReconnect(const SessionBase& base, const SessionBase& reconnectingBase)
{
    std::cout << "[FireServer] OnSessionReconnect - Session Reconnected.\n";

    delete sessions[*base.sid];
    sessions[*base.sid] = nullptr;
    return false;
}
