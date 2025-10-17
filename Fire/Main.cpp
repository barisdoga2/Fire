#define DUMP
#define CLIENT false
#define LOCAL_SERVER true
#define SERVER_PORT 54000U

#include <iostream>
#include <list>
#include <chrono>
#include <string>
#include <unordered_map>
#include <future>
#include <conio.h>
#ifdef DUMP
#include <windows.h>
#include <dbghelp.h>
#pragma comment(lib, "dbghelp.lib")
#endif

#include <lua.hpp>
#include <box2d/box2d.h>
#include <curl/curl.h>

#include "EasyBuffer.hpp"
#include "EasySocket.hpp"
#include "EasyIpAddress.hpp"
#include "EasyDB.hpp"
#include "EasyNet.hpp"
#include "EasyPeer.hpp"
#include "EasyPacket.hpp"


#if defined(_DEBUG) || LOCAL_SERVER
#if LOCAL_SERVER
#define SERVER_BUILD
#define SERVER_IP "127.0.0.1"
#else
#define SERVER_IP "31.210.43.142"
#endif
#else
#define RELEASE_BUILD
#define SERVER_BUILD
#define SERVER_IP "31.210.43.142"
#endif
#if CLIENT && !defined(RELEASE_BUILD)
#include "EasyPlayground.hpp"
#endif

EasyBufferManager bf(50U, 1472U);



namespace Server {
    class Server {
    public:
        using PeerList = std::unordered_map<Addr_t, EasyPeer>;

        // Flags
        bool running;

        // Net
        EasySocket sock;
        const unsigned short port;
        std::thread receive;
        std::thread update;

        // Resources
        PeerList peers;
        std::mutex receiveMutex;

        // DB
        EasyDB db;

        // World
        b2WorldId world;

        Server() : running(false), port(SERVER_PORT)
        {
            // Init Box2D
            {
                b2WorldDef worldDef = b2DefaultWorldDef();
                worldDef.gravity = { 0.0f, -10.0f };
                world = b2CreateWorld(&worldDef);

                b2BodyDef groundBodyDef = b2DefaultBodyDef();
                groundBodyDef.position = { 0.0f, -10.0f };
                b2BodyId groundId = b2CreateBody(world, &groundBodyDef);
                b2Polygon groundBox = b2MakeBox(50.0f, 10.0f);
                b2ShapeDef groundShapeDef = b2DefaultShapeDef();
                b2CreatePolygonShape(groundId, &groundShapeDef, &groundBox);

            }
        }

        ~Server()
        {
            Stop();
        }

        void B2Update(double _dt)
        {
            b2World_Step(world, (float)(_dt / 1000.0), 4);
        }

        void Update()
        {
            const double ups_constant = 1000.0 / 24.0;
            const double receive_constant = 1000.0 / 60.0;
            double ups_timer = 0.0, receive_timer = 0.0;
            std::chrono::high_resolution_clock::time_point lastTime = std::chrono::high_resolution_clock::now();
            while (true)
            {
                std::chrono::high_resolution_clock::time_point currentTime = std::chrono::high_resolution_clock::now();
                double elapsed_ms = std::chrono::duration<double, std::milli>(currentTime - lastTime).count();
                lastTime = currentTime;

                ups_timer += elapsed_ms;
                receive_timer += elapsed_ms;

                if (receive_timer >= receive_constant && receiveMutex.try_lock())
                {
                    ProcessReceived();
                    receive_timer = 0.0;
                    receiveMutex.unlock();
                }

                // Box2D Update
                if (ups_timer >= ups_timer)
                {
                    B2Update(ups_timer);
                    ups_timer = 0.0;
                }

                // Rest a bit
                std::this_thread::sleep_for(std::chrono::milliseconds(1U));
            }
        }

        void ProcessReceived()
        {
            for (auto peer = peers.begin(); peer != peers.end(); ++peer)
            {
                for (size_t i = 0U; i < peer->second.receiveBuffer.size(); i++)
                {
                    EasyBuffer* plainBuffer = peer->second.receiveBuffer[i];
                    EasyPacket asPacket(plainBuffer);

                    // Print Payload
                    {
                        std::string payload;
                        for (size_t ii = 0; ii < asPacket.PayloadSize() && ii < 10; ii++)
                            payload += std::to_string(*((unsigned char*)(asPacket.Payload() + ii))) + ((ii != 10 && ii != asPacket.PayloadSize()) ? ((ii == asPacket.PayloadSize() - 1U || ii == 10 - 1U) ? "" : " ") : ((ii == 10 && asPacket.PayloadSize() > 10) ? "..." : ""));
                        std::cout << "Server Receive Packet[" << (++i) << "] Payload: [" << payload << "]" << std::endl;
                    }

                    // Send Echo Reply
                    {
                        EasyBuffer* buff = bf.Get();

                        bool status = true;

                        if (status)
                            status = buff;

                        if (status)
                            status = EasyPacket::MakeCompressed(plainBuffer, buff);

                        if (status)
                            status = EasyPacket::MakeEncrypted(peer->second, buff);

                        if (status)
                        {
                            auto res = sock.send(buff->begin(), buff->m_payload_size + EasyPacket::HeaderSize(), peer->second.ip, peer->second.port);
                            std::cout << "Server Echo Reply Sent! Result: " << res << std::endl;
                        }
                        else
                        {
                            std::cout << "Server Error While Sending Echo Reply!" << std::endl;
                        }

                        bf.Free(buff);
                        bf.Free(plainBuffer);
                    }
                }
                peer->second.receiveBuffer.clear();
            }
        }

        void Receive()
        {
            using Clock = std::chrono::high_resolution_clock;
            using TS = Clock::time_point;
            using MS = std::chrono::milliseconds;
            const double flushPeriod = 1000.0 / 144.0;

            TS lastFlush, currentTime;
            PeerList peer_cache;
            while (true)
            {
                // Receive for 3ms every 7ms
                {
                    static TS nextReceive = Clock::now();
                    currentTime = Clock::now();
                    if (nextReceive < currentTime)
                    {
                        nextReceive = currentTime + MS(7U);
                        TS endTime = Clock::now() + MS(3U);
                        static EasyBuffer* buff = bf.Get();
                        while (Clock::now() < endTime)
                        {
                            if (!buff)
                                buff = bf.Get();
                            if (buff)
                            {
                                buff->m_payload_size = 0U;

                                EasyPeer host;
                                uint64_t ret = sock.receive(buff->begin(), buff->capacity(), buff->m_payload_size, host);
                                if (ret == WSAEISCONN && buff->m_payload_size > sizeof(SessionID_t) + sizeof(SequenceID_t) + IV_SIZE + 0U + TAG_SIZE)
                                {
                                    (void)ReceivePacket(buff, host, peer_cache);
                                }
                                else
                                {
                                    buff->m_payload_size = 0U;
                                }
                            }
                        }
                    }
                }

                // Flush every 4ms
                {
                    static TS nextFlush = Clock::now();
                    currentTime = Clock::now();
                    if (nextFlush < currentTime && receiveMutex.try_lock())
                    {
                        nextFlush = currentTime + MS(4U);
                        for (auto peer = peer_cache.begin(); peer != peer_cache.end(); ++peer)
                        {
                            if (peer->second.receiveBuffer.size() > 0U)
                            {
                                if (auto r_peer = peers.find(peer->second.addr); r_peer != peers.end())
                                {
                                    r_peer->second.receiveBuffer.insert(r_peer->second.receiveBuffer.end(), peer->second.receiveBuffer.begin(), peer->second.receiveBuffer.end());
                                }
                                else
                                {
                                    std::pair<PeerList::iterator, bool> newPeer = peers.insert({ peer->first, peer->second });
                                    newPeer.first->second.receiveBuffer.insert(newPeer.first->second.receiveBuffer.end(), peer->second.receiveBuffer.begin(), peer->second.receiveBuffer.end());
                                }
                            }
                            peer->second.receiveBuffer.clear();
                        }
                        receiveMutex.unlock();
                    }
                }

                // Rest a bit
                std::this_thread::sleep_for(std::chrono::milliseconds(1U));
            }
        }

        bool ReceivePacket(EasyBuffer* buff, EasyPeer& host, PeerList& peer_cache)
        {
            EasyPeer& actualHost = host;

            PeerList::iterator cache_peer = peer_cache.find(host.addr);

            EasyPacket packet(buff);
            memcpy(actualHost.secret.second.data(), packet.IV(), IV_SIZE);

            SessionID_t session_id = *packet.SessionID();

            bool status = true;
            if (cache_peer == peer_cache.end())
            {
                actualHost.session_id = session_id;
                status = ReadKeyFromMYSQL(*packet.SessionID(), actualHost.secret.first);
            }
            else if(session_id == cache_peer->second.session_id)
            {
                actualHost = cache_peer->second;
            }
            else
            {
                status = false;
            }

            if (status)
                status = EasyPacket::MakeDecrypted(host, buff);

            EasyBuffer* buff2 = bf.Get();
            if (status)
                status = buff2;

            if (status)
                status = EasyPacket::MakeDecompressed(buff, buff2);

            if (status)
            {
                if (cache_peer == peer_cache.end())
                {
                    std::pair<PeerList::iterator, bool> res = peer_cache.insert({ host.addr, host });
                    (*res.first).second.receiveBuffer.push_back(buff2);
                }
                else
                {
                    cache_peer->second.receiveBuffer.push_back(buff2);
                }
            }
            else
            {
                std::cout << "Server Error while processing incoming packet!" << std::endl;
                bf.Free(buff2);
            }

            return status;
        }

        bool Start()
        {
            if (running)
                return false;

            std::cout << "Server is starting..." << std::endl;

            if (sock.bind(port) != WSAEISCONN)
                return false;

            running = true;
            receive = std::thread(&Server::Receive, this);
            update = std::thread(&Server::Update, this);

            return true;
        }

        void Stop()
        {
            if (!running)
                return;

            std::cout << "Server is stopping..." << std::endl;

            running = false;

            if (receive.joinable())
                receive.join();
            if (update.joinable())
                update.join();
        }

        bool ReadKeyFromMYSQL(const SessionID_t& session_id, Key_t& key)
        {
            bool ret = false;
            sql::PreparedStatement* stmt = db.PrepareStatement("SELECT * FROM sessions WHERE id=? LIMIT 1;");
            stmt->setUInt(1, session_id);
            sql::ResultSet* res = stmt->executeQuery();
            while (res->next())
            {
                memcpy(key.data(), res->getString(3).c_str(), KEY_SIZE);
                ret = true; // If valid_until < current_timestamp
                break;
            }
            delete res;
            delete stmt;
            return ret;
        }

    };

}

namespace ClientTest {
    std::string key = "fbdRjmU4rvENzNCy";
    EasyPeer client;
    EasySocket socket;
    uint8_t ctr = (uint8_t)'*';
    uint8_t payload_size = 10U;

    struct ClientTestInitializer
    {
        ClientTestInitializer()
        {
            //client.user_id = 1U;
            client.session_id = 2U;
            client.sequence_id_in = 0U;
            client.sequence_id_out = 0U;
            memcpy(client.secret.first.data(), key.c_str(), KEY_SIZE);
        }
    };
    ClientTestInitializer clientTestInitializer;

    int ClientWebRequest(lua_State* L)
    {
        CURL* curl = curl_easy_init();
        if (!curl) {
            std::cout << "Failed to initialize CURL\n";
            return 1;
        }

        std::string response;
        std::string url = "https://barisdoga.com/index.php";
        std::string postData = "username=barisdoga&password=123&login=1";
        std::string prefix = "<textarea name=\"jwt\" readonly>";
        std::string prefix2 = "<div class=\"msg\">";
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
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK)
        {
            std::cout << "Error! Login failed! CURL error: " << curl_easy_strerror(res) << std::endl;
        }
        else
        {
            if (size_t index = response.find(prefix); index != std::string::npos)
            {
                response = response.substr(index + prefix.length());
                response = response.substr(0, response.find("</textarea>"));
                SessionID_t sessionID = static_cast<SessionID_t>(std::stoul(response.substr(0, response.find_first_of(":"))));
                response = response.substr(response.find_first_of(":") + 1U);
                uint32_t userID = static_cast<uint32_t>(std::stoul(response.substr(0, response.find_first_of(":"))));
                response = response.substr(response.find_first_of(":") + 1U);
                std::string key = response;

                client.session_id = sessionID;
                memcpy(client.secret.first.data(), key.data(), KEY_SIZE);

                std::cout << "Login OK! SessionID: " << sessionID << ", UserID: " << userID << ", Key: " << key << std::endl;
            }
            else
            {
                if (size_t index = response.find(prefix2); index != std::string::npos)
                {
                    response = response.substr(index + prefix2.length());
                    response = response.substr(0, response.find("</div>"));
                    std::cout << "Error! Login failed! Message: " << response << std::endl;
                }
                else
                {
                    std::cout << "Error! Login response parse failed!"  << std::endl;
                }
            }
        }

        curl_easy_cleanup(curl);
        return 0;
    }

    int ClientPacket(lua_State* L)
    {
        EasyBuffer* buff = bf.Get();
        EasyBuffer* buff2 = bf.Get();
        EasyBuffer* buff3 = bf.Get();

        if (buff && buff2 && buff3)
        {
            // Sample Payload
            Payload_t payload;
            for (uint8_t i = 0U; i < payload_size; i++)
                payload.push_back(ctr);
            EasyPacket plainPacket(buff);
            memcpy(plainPacket.Payload(), payload.data(), payload.size());
            buff->m_payload_size = payload.size();

            // Prepare Packet
            bool result = true;
            if (result)
                result = EasyPacket::MakeCompressed(buff, buff2);

            if (result)
                result = EasyPacket::MakeEncrypted(client, buff2);

            // Send Packet
            if (result)
            {
                auto res = socket.send(buff2->begin(), sizeof(SessionID_t) + sizeof(SequenceID_t) + IV_SIZE + buff2->m_payload_size, EasyIpAddress::resolve(SERVER_IP), SERVER_PORT);
                std::cout << "Client Packet Send! Result: " << res << std::endl;
            }
            else
            {
                std::cout << "Client Packet Send! Error while packet create! " << std::endl;
            }
        }
        bf.Free(buff);
        bf.Free(buff2);
        bf.Free(buff3);
        return 0;
    }

    int ClientReceive(lua_State* L)
    {
        EasyBuffer* buff = bf.Get();
        EasyBuffer* buff2 = bf.Get();
        if (buff && buff2)
        {
            EasyIpAddress receiveIP;
            unsigned short receivePort;
            auto res = socket.receive(buff->begin(), buff->capacity(), buff->m_payload_size, receiveIP, receivePort);
            std::cout << "Client Packet Receive! Result: " << res << std::endl;

            bool status = (res == WSAEISCONN);

            if (status)
                status = EasyPacket::MakeDecrypted(client, buff);

            if (status)
                status = EasyPacket::MakeDecompressed(buff, buff2);

            if (status)
            {
                EasyPacket asPacket(buff2);

                std::string asString;
                for (size_t ii = 0; ii < buff2->m_payload_size && ii < 10; ii++)
                    asString += std::to_string(*((unsigned char*)(asPacket.Payload() + ii))) + ((ii != 10 && ii != asPacket.PayloadSize()) ? " " : ((ii == 10 && asPacket.PayloadSize() > 10) ? "..." : ""));
                std::cout << "    Payload: [" << asString << "]" << std::endl;
                ctr = *asPacket.Payload();
            }
            else
            {
                std::cout << "Client Error While Processing Received Packet!" << std::endl;
            }
        }
        bf.Free(buff);
        bf.Free(buff2);
        return 0;
    }
    
    int ClientBoth(lua_State* L)
    {
        ClientWebRequest(L);
        ClientPacket(L);
        std::this_thread::sleep_for(std::chrono::milliseconds(1500U));
        ClientReceive(L);
        return 0;
    }
}

void LUAListen(bool& running)
{
    std::cout << "LUA commander is starting..." << std::endl;

    lua_State* L;

    L = luaL_newstate();
    luaL_openlibs(L);
    lua_register(L, "W", ClientTest::ClientWebRequest);
    lua_register(L, "P", ClientTest::ClientPacket);
    lua_register(L, "R", ClientTest::ClientReceive);
    lua_register(L, "B", ClientTest::ClientBoth);

    std::string input = "", input2 = "";
    while (running)
    {
        if (input.length() != 0)
        {
            if (luaL_dostring(L, input.c_str()) != LUA_OK)
            {
                std::cerr << "Lua error: " << lua_tostring(L, -1) << std::endl;
                lua_pop(L, 1);
            }
            input = "";

        }
        if(_kbhit()) 
        { 
            char c = (char)_getch();
            if (c == '\r' || c == '\n') {
                if (input2 == "exit") {
                    running = false;
                    break;
                }
                input = input2;
                std::cout << std::endl;
                input2.clear();
            }
            else if (c == '\b') {
                if (!input2.empty()) {
                    input2.pop_back();
                    std::cout << "\b \b";
                }
            }
            else {
                input2.push_back(c);
                std::cout << c;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

    }

    std::cout << "LUA commander is closing..." << std::endl;

    lua_close(L);
}

#ifdef DUMP
namespace Dump {
    void CreateMiniDump(EXCEPTION_POINTERS* e) {
        SYSTEMTIME st;
        GetSystemTime(&st);
        char filename[MAX_PATH];
        sprintf_s(filename, "Crash_%04d%02d%02d_%02d%02d%02d.dmp",
            st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

        HANDLE hFile = CreateFileA(filename, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL, nullptr);

        if ((hFile != nullptr) && (hFile != INVALID_HANDLE_VALUE)) {
            MINIDUMP_EXCEPTION_INFORMATION info;
            info.ThreadId = GetCurrentThreadId();
            info.ExceptionPointers = e;
            info.ClientPointers = FALSE;

            MiniDumpWriteDump(
                GetCurrentProcess(),
                GetCurrentProcessId(),
                hFile,
                MiniDumpWithFullMemory, // or MiniDumpNormal
                e ? &info : nullptr,
                nullptr,
                nullptr
            );

            CloseHandle(hFile);
            std::cout << "Crash dump written: " << filename << std::endl;
        }
    }

    LONG WINAPI ExceptionHandler(EXCEPTION_POINTERS* e) {
        std::cerr << "Unhandled exception caught! Creating dump...\n";
        CreateMiniDump(e);
        return EXCEPTION_EXECUTE_HANDLER;
    }

}
#endif

int main()
{
#ifdef DUMP
    SetUnhandledExceptionFilter(Dump::ExceptionHandler);
#endif
    bool luaRunning = true;
    std::thread luaThread = std::thread([&]() { LUAListen(luaRunning); });

#ifdef SERVER_BUILD
    Server::Server server;
    if (server.Start())
    {
#if !(CLIENT && !defined(RELEASE_BUILD))
        while (luaRunning && server.running)
            std::this_thread::sleep_for(std::chrono::milliseconds(10000U));
#endif
    }
#endif
#if CLIENT && !defined(RELEASE_BUILD)
    if (EasyDisplay display({ 1024,768 }); display.Init())
    {
        if (EasyPlayground playground(display); playground.Init())
        {
            while (playground.OneLoop());
            luaRunning = false;
        }
    }
#endif
#if !(CLIENT && !defined(RELEASE_BUILD)) || !defined(SERVER_BUILD)
    while (luaRunning)
        std::this_thread::sleep_for(std::chrono::milliseconds(10000U));
#endif
    luaRunning = false;
#ifdef SERVER_BUILD
    server.Stop();
#endif
    if (luaThread.joinable())
        luaThread.join();
	return 0;
}

