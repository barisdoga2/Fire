#include "pch.h"

#include "BaseServer.hpp"

#include <iostream>
#include <vector>
#include <array>
#include <unordered_map>
#include <chrono>
#include <atomic>
#include <sstream>
#include <random>
#include <optional>
#include <filesystem>
#include <ctime>
#define CROW_ENABLE_SSL
#include <crow.h>
#include <filesystem>
#include <fstream>

#include "EasyIpAddress.hpp"
#include "EasySocket.hpp"
#include "EasyBuffer.hpp"
#include "EasySerializer.hpp"
#include "EasyPacket.hpp"
#include "EasyNet.hpp"
#include "../FireRender/EasyUtils.hpp"

#include <filesystem>
#include <fstream>
#include <unordered_map>


#include <openssl/sha.h>
#include <random>
#include <sstream>
#include <iomanip>

namespace fs = std::filesystem;


static inline std::string Sha256(const std::string& input) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((const unsigned char*)input.c_str(), input.size(), hash);

    std::ostringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}

static inline std::string RandomString(size_t len) {
    static const char chars[] =
        "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    thread_local std::mt19937 rng{ std::random_device{}() };
    std::uniform_int_distribution<> dist(0, sizeof(chars) - 2);

    std::string s;
    for (size_t i = 0; i < len; i++) {
        s += chars[dist(rng)];
    }
    return s;
}

static inline std::string RenderPage(const std::string& message, const std::string& jwt) {
    std::ostringstream html;
    html <<
        R"(<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<title>FireWeb Login / Register</title>
<style>
body { font-family: Arial; text-align: center; margin-top: 60px; }
input { padding: 8px; margin: 5px; width: 200px; }
button { padding: 8px 15px; margin: 5px; }
.msg { margin-top: 20px; font-weight: bold; }
textarea { width: 80%; height: 100px; margin-top: 10px; }
</style>
</head>
<body>

<h1><a href="Fire.7z">Download Client</a></h1>

<h2>FireWeb Login / Register</h2>
<form method="POST" action="/FireFS/index.php">
<input type="text" name="username" placeholder="Username" required><br>
<input type="password" name="password" placeholder="Password" required><br>
<button type="submit" name="login">Login</button>
<button type="submit" name="register">Register</button>
</form>

<div class="msg">)" << message << R"(</div>)";

    if (!jwt.empty()) {
        html << R"(
<h3>JWT Token</h3>
<textarea readonly>)" << jwt << R"(</textarea>)";
    }

    html << R"(</body></html>)";
    return html.str();
}

namespace fs = std::filesystem;

static inline const fs::path ROOT = fs::absolute("release_api");

static inline std::string mime_type(const fs::path& p) {
    static std::unordered_map<std::string, std::string> map = {
        {".html","text/html"},
        {".js","application/javascript"},
        {".css","text/css"},
        {".json","application/json"},
        {".png","image/png"},
        {".jpg","image/jpeg"},
        {".jpeg","image/jpeg"},
        {".ico","image/x-icon"},
        {".txt","text/plain"},
        {".dll","application/octet-stream"},
        {".exe","application/octet-stream"},
        {".glsl","text/plain"}
    };

    auto ext = p.extension().string();
    for (auto& c : ext) c = char(tolower(c));

    auto it = map.find(ext);
    if (it != map.end()) { return it->second; }
    return "application/octet-stream";
}

static inline  bool safe_path(const fs::path& base, const fs::path& target) {
    auto canonBase = fs::weakly_canonical(base);
    auto canonTarget = fs::weakly_canonical(target);
    return true;// std::mismatch(canonBase.begin(), canonBase.end(), canonTarget.begin()).first == canonBase.end();
}



class Session {
public:
    SessionID_t sid;
    Addr_t addr;
    Key_t key;
    Key_Expt_t key_expiry;
    SequenceID_t seqid_in, seqid_out;
    Timestamp_t lastReceive;
    Session() : sid(0U), addr(0U), key(16U), key_expiry(), seqid_in(0U), seqid_out(0U), lastReceive()
    {
    }
    Session(const Session& session) : sid(session.sid), addr(session.addr), key(session.key), key_expiry(session.key_expiry), seqid_in(session.seqid_in), seqid_out(session.seqid_out), lastReceive(session.lastReceive)
    {
    }
    Session(const SessionID_t& sid, const Addr_t& addr, const Key_t& key, const Key_Expt_t& key_expiry, const SequenceID_t& seqid_in, const SequenceID_t& seqid_out, const Timestamp_t& lastReceive) : sid(sid), addr(addr), key(key), key_expiry(key_expiry), seqid_in(seqid_in), seqid_out(seqid_out), lastReceive(lastReceive)
    {
    }

    SessionBase GetBase()
    {
        return {&sid, &addr, &key, &key_expiry, &lastReceive };
    }
};

class ReceiveManager;
class SendManager;
class SessionManager;
class ServerContext {
public:
    const bool encryption, compression;
    EasySocket* sck;
    EasyBufferManager* bufferMan;
    SessionManager* sessionMan;
    ReceiveManager* receiveMan;
    SendManager* sendMan;
    ServerCallback* cbk;

    ServerCache_t receiveCache;
    ServerCache_t sendCache;

    ServerContext(EasyBufferManager* bm, ServerCallback* cbk, bool encryption, bool compression) : bufferMan(bm), cbk(cbk), encryption(encryption), compression(compression), sessionMan(), receiveMan(), sendMan(), sck()
    {

    }

    ~ServerContext()
    {

    }
};

class SessionManager {
private:
    std::vector<SessionID_t> sessionIDs;
    std::array<Session*, MAX_SESSIONS> sessions{ nullptr };

    ServerContext* cntx;

public:
    SessionManager(ServerContext* cntx) : cntx(cntx)
    {

    }

    ~SessionManager()
    {
        for (const SessionID_t& sid : sessionIDs)
        {
            delete sessions[sid];
            sessions[sid] = nullptr;
        }
        sessionIDs.clear();
    }

    void Update(double dt)
    {

    }

    Session* CreateSession(const SessionID_t& sid, const Addr_t& addr, const Timestamp_t& lastReceive, EasyBuffer* buffer)
    {
        if (sessions[sid])
            return nullptr;
        if (Session* session = new Session(sid, addr, {}, {}, 0U, 0U, lastReceive); cntx->cbk->OnSessionCreate(session->GetBase(), buffer))
        {
            sessions[sid] = session;
            sessionIDs.push_back(sid);
            return session;
        }
        else
        {
            delete session;
        }
        return nullptr;
    }

    void DestroySession(SessionID_t sid, std::string disconnectMessage = "")
    {
        if (!sessions[sid])
            return;
        cntx->cbk->OnSessionDestroy(sessions[sid]->GetBase(), disconnectMessage);
        sessionIDs.erase(std::find(sessionIDs.begin(), sessionIDs.end(), sid));
        delete sessions[sid];
        sessions[sid] = nullptr;
    }

    Session* GetSession(SessionID_t sid)
    {
        return sessions[sid];
    }

};

class ReceiveManager {
    class RawPacket {
    public:
        SessionID_t sid;
        Addr_t addr;
        EasyBuffer* buff;
        Timestamp_t recv;
        RawPacket(const SessionID_t sid, const Addr_t addr, EasyBuffer* buff, const Timestamp_t recv) : sid(sid), addr(addr), buff(buff), recv(recv)
        {

        }
    };

    ServerContext* cntx;
    EasyBuffer* receiveBuffer;
    EasyBuffer* decompressBuffer;

public:
    ReceiveManager(ServerContext* cntx) : cntx(cntx), receiveBuffer(), decompressBuffer()
    {
        while (!receiveBuffer)
            receiveBuffer = cntx->bufferMan->Get();
        while (!decompressBuffer)
            decompressBuffer = cntx->bufferMan->Get();
    }

    ~ReceiveManager()
    {
        if(receiveBuffer)
            cntx->bufferMan->Free(receiveBuffer);
        if (decompressBuffer)
            cntx->bufferMan->Free(decompressBuffer);
    }

    void Update(double dt)
    {
        for (uint8_t i = 0U; i < 16U; i++)
        {
            if (RawPacket* rcv = ReceiveOne(); rcv)
            {
                Session* session = cntx->sessionMan->GetSession(rcv->sid);

                if (!session)
                {
                    session = cntx->sessionMan->CreateSession(rcv->sid, rcv->addr, rcv->recv, rcv->buff);
                }
                else
                {
                    if (session->addr != rcv->addr)
                    {
                        session = cntx->cbk->OnSessionReconnect(session->GetBase(), { &rcv->sid, &rcv->addr, &session->key, &session->key_expiry, &rcv->recv }) ? session : nullptr;
                    }
                    else
                    {

                    }
                    if (session)
                        ProcessReceived(session, rcv);
                    else
                        cntx->bufferMan->Free(rcv->buff);
                }
            }
        }
    }

private:
    ReceiveManager::RawPacket* ReceiveOne()
    {
        if (!receiveBuffer)
            receiveBuffer = cntx->bufferMan->Get();
        if (!receiveBuffer)
            return nullptr;

        uint64_t addr;
        receiveBuffer->reset();
        if (uint64_t ret = cntx->sck->receive(receiveBuffer->begin(), receiveBuffer->capacity(), receiveBuffer->m_payload_size, addr); ret == WSAEISCONN)
        {
            if (receiveBuffer->m_payload_size >= EasyPacket::MinimumSize())
            {
                EasyPacket packet(receiveBuffer);
                SessionID_t sid = *packet.SessionID();
                if (IS_SESSION(sid))
                {
                    ReceiveManager::RawPacket* ret = new ReceiveManager::RawPacket(sid, addr, receiveBuffer, Clock::now());
                    receiveBuffer = nullptr;
                    return ret;
                }
            }
        }
        return nullptr;
    }

    bool ProcessReceived(Session* session, RawPacket* rcv)
    {
        bool ret = false;
        EasyPacket pck(rcv->buff);
        CryptData crypt(session->sid, session->seqid_in, session->seqid_out, session->key);
        if (pck.MakeDecrypted(crypt, rcv->buff))
        {
            decompressBuffer->reset();
            if (pck.MakeDecompressed(rcv->buff, decompressBuffer))
            {
                std::vector<EasySerializeable*> cache;
                if (MakeDeserialized(decompressBuffer, cache))
                {
                    cntx->receiveCache[session->sid].insert(cntx->receiveCache[session->sid].end(), cache.begin(), cache.end());
                    session->seqid_in++;
                    session->lastReceive = rcv->recv;
                    ret = true;
                }
            }
        }
        cntx->bufferMan->Free(rcv->buff);
        return ret;
    }

    std::vector<EasySerializeable*> ProcessReceived(const CryptData& crypt, const RawPacket* rcv)
    {
        std::vector<EasySerializeable*> ret{};
        EasyPacket pck(rcv->buff);
        if (pck.MakeDecrypted(crypt, rcv->buff))
        {
            decompressBuffer->~EasyBuffer();
            if (pck.MakeDecompressed(rcv->buff, decompressBuffer))
            {
                if (MakeDeserialized(decompressBuffer, ret))
                {

                }
            }
        }
        cntx->bufferMan->Free(rcv->buff);
        return ret;
    }

public:
    std::vector<EasySerializeable*> Process(const CryptData& crypt, EasyBuffer* rcvBuff)
    {
        std::vector<EasySerializeable*> ret{};
        EasyPacket pck(rcvBuff);
        if (pck.MakeDecrypted(crypt, rcvBuff))
        {
            decompressBuffer->reset();
            if (pck.MakeDecompressed(rcvBuff, decompressBuffer))
            {
                if (MakeDeserialized(decompressBuffer, ret))
                {

                }
            }
            cntx->bufferMan->Free(rcvBuff);
        }
        return ret;
    }
};

class SendManager {
    ServerContext* cntx;
    EasyBuffer* serializationBuffer;
    EasyBuffer* sendBuffer;

public:
    SendManager(ServerContext* cntx) : cntx(cntx), serializationBuffer(), sendBuffer()
    {
        while (!serializationBuffer)
            serializationBuffer = cntx->bufferMan->Get();
        while (!sendBuffer)
            sendBuffer = cntx->bufferMan->Get();
    }

    ~SendManager()
    {
        if(serializationBuffer)
            cntx->bufferMan->Free(serializationBuffer);
        if(sendBuffer)
            cntx->bufferMan->Free(sendBuffer);
    }

    void Update(double dt)
    {
        for (auto& [sid, cache] : cntx->sendCache)
        {
            SendMultiple(sid, cache);
            for (auto& obj : cache)
                delete obj;
            cache.clear();
        }
        cntx->sendCache.clear();
    }

private:
    void SendMultiple(SessionID_t sid, const std::vector<EasySerializeable*>& cache)
    {
        if (Session* session = cntx->sessionMan->GetSession(sid); ((cache.size() > 0U) && (session)))
        {
            serializationBuffer->reset();
            if (MakeSerialized(serializationBuffer, cache))
            {
                sendBuffer->reset();
                if (EasyPacket::MakeCompressed(serializationBuffer, sendBuffer))
                {
                    if (EasyPacket::MakeEncrypted({ sid, session->seqid_in, session->seqid_out, session->key }, sendBuffer))
                    {
                        if (uint64_t res = cntx->sck->send(sendBuffer->begin(), sendBuffer->m_payload_size + EasyPacket::HeaderSize(), session->addr); res == WSAEISCONN)
                        {
                            session->seqid_out++;
                        }
                    }
                }
            }
        }
    }

    void SendMultipleTo(SessionID_t sid, const std::vector<EasySerializeable*>& cache, const Addr_t& addr, const SequenceID_t& seqid_in, const SequenceID_t& seqid_out, const Key_t& key)
    {
        if (cache.size() > 0U)
        {
            serializationBuffer->reset();
            if (MakeSerialized(serializationBuffer, cache))
            {
                sendBuffer->reset();
                if (EasyPacket::MakeCompressed(serializationBuffer, sendBuffer))
                {
                    if (EasyPacket::MakeEncrypted({ sid, seqid_in, seqid_out, key }, sendBuffer))
                    {
                        if (uint64_t res = cntx->sck->send(sendBuffer->begin(), sendBuffer->m_payload_size + EasyPacket::HeaderSize(), addr); res == WSAEISCONN)
                        {

                        }
                    }
                }
            }
        }
    }

    friend class BaseServer;
};

BaseServer::BaseServer() : cntx(), running(false)
{

}

BaseServer::~BaseServer()
{
    Stop();
}

std::vector<EasySerializeable*> BaseServer::Process(const CryptData& crypt, EasyBuffer* buffer)
{
    return cntx->receiveMan->Process(crypt, buffer);
}

void BaseServer::Send(SessionID_t sid, const std::vector<EasySerializeable*>& objs, const Addr_t& addr, const SequenceID_t& seqid_in, const SequenceID_t& seqid_out, const Key_t& key)
{
    cntx->sendMan->SendMultipleTo(sid, objs, addr, seqid_in, seqid_out, key);
}

void BaseServer::Send(SessionID_t sid, const std::vector<EasySerializeable*>& objs)
{
    cntx->sendMan->SendMultiple(sid, objs);
}

void BaseServer::DestroySession(SessionID_t sid, std::string disconnectMessage)
{
    cntx->sessionMan->DestroySession(sid, disconnectMessage);
}

struct AuthResult
{
    std::string message;
    std::string jwt;
};

AuthResult HandleAuth(const crow::request& req, SqLite3& db)
{
    crow::query_string params(("?" + req.body).c_str());

    auto get = [&](const char* key) -> std::string {
        if (auto v = params.get(key))
            return v;
        return {};
        };

    auto has = [&](const char* key) -> bool {
        return req.body.find(key) != std::string::npos;
        };

    std::string user = get("username");
    std::string pass = get("password");

    AuthResult res;

    // ------------------------
    // Login
    // ------------------------
    if (has("login"))
    {
        auto rows = db.Query("SELECT id, password FROM users WHERE username='" + user + "';"
        );

        if (rows.empty() || rows[0][1] != Sha256(pass))
        {
            res.message = "Wrong username or password!";
        }
        else
        {
            int uid = std::stoi(rows[0][0]);
            std::string key = RandomString(16);
            int validUntil = (int)time(nullptr) + 3600;

            db.Execute("DELETE FROM sessions WHERE user_id=" + std::to_string(uid) + ";");
            db.Execute("INSERT INTO sessions (user_id, session_key, valid_until) VALUES (" +std::to_string(uid) + ",'" + key + "'," + std::to_string(validUntil) + ");");

            res.jwt = std::to_string(uid) + ":" + key;
            res.message = "Login successful!";
        }
    }

    if (has("register"))
    {
        std::string hash = Sha256(pass);
        bool ok = db.Execute("INSERT INTO users (username,password) VALUES ('" +user + "','" + hash + "');");

        if (!ok)
        {
            res.message = "Username already exists!";
        }
        else
        {
            auto id = db.QueryInt("SELECT last_insert_rowid();");
            db.Execute("INSERT INTO user_stats (user_id) VALUES (" +std::to_string(*id) + ");");
            res.message = "Register successful!";
        }
    }

    return res;
}


bool BaseServer::Start(std::string apiPath, std::string apiDomain, std::string apiCrt, std::string apiKey, std::string apiIP, uint16_t apiPort, SqLite3& db, EasyBufferManager* bm, const unsigned short port, ServerCallback* cbk, bool encryption, bool compression)
{
    if (IsRunning())
        return false;

    std::cout << "[BaseServer] Start - Starting...\n";
    cntx = new ServerContext(bm, cbk, encryption, compression);
    
    cntx->sck = new EasySocket();
    if (cntx->sck->bind(port, EasyIpAddress::Any) != WSAEISCONN)
    {
        std::cout << "[BaseServer] Start - Failed to bind the port!" << std::endl;
        return IsRunning();
    }

    running = true;
    BaseServer::instance = this;

    cntx->sessionMan = new SessionManager(cntx);
    cntx->receiveMan = new ReceiveManager(cntx);
    cntx->sendMan = new SendManager(cntx);

    if (!cntx->cbk->OnServerStart())
        Stop();
    else
        std::cout << "[BaseServer] Start - Started.\n";

    // Web API
    {
        crow::App<> app;
        
        std::string domain = "/" + apiDomain + "/";

        CROW_ROUTE(app, "/<string>/<string>").methods(crow::HTTPMethod::POST)(
            [&](const crow::request& req, std::string path, std::string page) {
                if (path.starts_with("/" + apiDomain + "/.index.php")) {
                    auto result = HandleAuth(req, db);
                    return crow::response(200, RenderPage(result.message, result.jwt));
                }
                return crow::response(404);
            }
            );

        CROW_ROUTE(app, "/<string>/<string>").methods(crow::HTTPMethod::GET)(
            [&](const crow::request& req, std::string path, std::string page) {
                if (path.starts_with("/" + apiDomain + "/.index.php")) {
                    auto result = HandleAuth(req, db);
                    return crow::response(200, RenderPage(result.message, result.jwt));
                }
                else if (path.starts_with("/" + apiDomain + "/")) {
                    fs::path filePath = apiPath + path;

                    if (!safe_path(ROOT, filePath) ||
                        !fs::exists(filePath) ||
                        fs::is_directory(filePath))
                    {
                        return crow::response(404, "404 Not Found");
                    }

                    std::ifstream in(filePath, std::ios::binary);
                    if (!in) {
                        return crow::response(500, "Read error");
                    }

                    std::string body(
                        (std::istreambuf_iterator<char>(in)),
                        std::istreambuf_iterator<char>()
                    );

                    crow::response res;
                    res.code = 200;
                    res.body = std::move(body);
                    res.set_header("Content-Type", mime_type(filePath));
                    res.set_header("Cache-Control", "no-cache, no-store, must-revalidate");
                    res.set_header("Pragma", "no-cache");
                    res.set_header("Expires", "0");

                    return res;
                }
                return crow::response(404);
            }
            );

        app
            .ssl_file(apiCrt, apiKey)
            .bindaddr(apiIP)
            .port(apiPort)
            .multithreaded()
            .run();
    }

    return IsRunning();
}

void BaseServer::Stop(std::string shutdownMessage)
{
    if (!IsRunning())
        return;
    std::cout << "[BaseServer] Stop - Stopping...\n";

    cntx->cbk->OnServerStop(shutdownMessage);

    running = false;
    BaseServer::instance = nullptr;

    delete cntx->sessionMan;
    cntx->sessionMan = nullptr;

    delete cntx->receiveMan;
    cntx->receiveMan = nullptr;

    delete cntx->sendMan;
    cntx->sendMan = nullptr;

    delete cntx->sck;
    cntx->sck = nullptr;

    delete cntx;
    cntx = nullptr;

    std::cout << "[BaseServer] Stop - Stopped.\n";
}

void BaseServer::Update(double dt)
{
    if (!IsRunning())
        return;

    cntx->receiveMan->Update(dt);
    cntx->sendMan->Update(dt);
    cntx->sessionMan->Update(dt);
}

bool BaseServer::IsRunning()
{
    return running;
}

ServerCache_t& BaseServer::GetReceiveCache()
{
    return cntx->receiveCache;
}

ServerCache_t& BaseServer::GetSendCache()
{
    return cntx->sendCache;
}