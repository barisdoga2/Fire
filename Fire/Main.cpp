#define DUMP
#define CLIENT true
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
#include <curl/curl.h>

#include "EasyBuffer.hpp"
#include "EasySocket.hpp"
#include "EasyIpAddress.hpp"
#include "EasyDB.hpp"
#include "EasyNet.hpp"
#include "EasyPeer.hpp"
#include "EasyPacket.hpp"
#include "EasySerializer.hpp"
#include "World.hpp"
#include "Serializer.hpp"

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
        using PeerList = std::unordered_map<SessionID_t, EasyPeer>;

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
        World world;

        Server() : running(false), port(SERVER_PORT), world()
        {
            
        }

        ~Server()
        {
            Stop();
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
                    world.Update(ups_timer);
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
                for (size_t i = 0U; i < peer->second.receive.size(); i++)
                {
                    EasyNetObj* o = peer->second.receive[i];
                    if (o->packetID == HELLO)
                        std::cout << "Server Received Hello! Message: " << ((pHello*)o)->message << "\n";

                    // Send Echo Response
                    std::vector<EasyNetObj*> cache;
                    EasyBuffer* plainBuffer = bf.Get();
                    EasyBuffer* buff = bf.Get();

                    bool status = plainBuffer && buff;
                    if(status)
                    {
                        pHello echo("Hi, Im server :)");
                        cache.push_back(&echo);

                        MakeSerialized(plainBuffer, cache);
                    }

                    if (status)
                        status = EasyPacket::MakeCompressed(plainBuffer, buff);
                    
                    if (status)
                        status = EasyPacket::MakeEncrypted(peer->second, buff);

                    if (status)
                    {
                        auto res = sock.send(buff->begin(), buff->m_payload_size + EasyPacket::HeaderSize(), peer->second);
                        std::cout << "Server Echo Reply Sent! Result: " << res << std::endl;
                    }
                    else
                    {
                        std::cout << "Server Error While Sending Echo Reply!" << std::endl;
                    }

                    bf.Free(buff);
                    bf.Free(plainBuffer);
                    delete o;
                }
                peer->second.receive.clear();
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
                // Clear cache every 10000ms
                {
                    static TS nextClear = Clock::now();
                    currentTime = Clock::now();
                    if (nextClear < currentTime)
                    {
                        for (auto it = peer_cache.begin(); it != peer_cache.end(); )
                        {
                            if (!it->second.alive())
                            {
                                std::cout << "Peer cache, peer dead\n";
                                it = peer_cache.erase(it);
                            }
                            else
                            {
                                ++it;
                            }
                        }
                        nextClear = currentTime + MS(10000U);
                    }
                }

                // Receive for 3ms every 7ms
                {
                    static TS nextReceive = Clock::now();
                    currentTime = Clock::now();
                    if (nextReceive < currentTime)
                    {
                        nextReceive = currentTime + MS(7U);
                        TS endTime = Clock::now() + MS(3U);
                        static EasyBuffer* buff = bf.Get();
                        EasyPeer host;
                        while (Clock::now() < endTime)
                        {
                            if (!buff)
                                buff = bf.Get();
                            if (buff)
                            {
                                buff->m_payload_size = 0U;

                                uint64_t ret = sock.receive(buff->begin(), buff->capacity(), buff->m_payload_size, host);
                                if (ret == WSAEISCONN && buff->m_payload_size > sizeof(SessionID_t) + sizeof(SequenceID_t) + IV_SIZE + 0U + TAG_SIZE)
                                {
                                    host.lastReceive = currentTime;

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
                        FlushCache(peer_cache);
                        receiveMutex.unlock();
                    }
                }

                // Rest a bit
                std::this_thread::sleep_for(std::chrono::milliseconds(1U));
            }
        }

        void FlushCache(PeerList& peer_cache)
        {
            for (auto peer = peer_cache.begin(); peer != peer_cache.end(); ++peer)
            {
                if (peer->second.receive.size() > 0U)
                {
                    if (auto r_peer = peers.find(peer->second.session_id); r_peer != peers.end())
                    {
                        r_peer->second.receive.insert(r_peer->second.receive.end(), peer->second.receive.begin(), peer->second.receive.end());
                    }
                    else
                    {
                        peers.insert({ peer->first, peer->second });
                    }
                }
                peer->second.receive.clear();
            }
        }

        bool ReceivePacket(EasyBuffer* buff, EasyPeer& host, PeerList& peer_cache)
        {
            static EasyBuffer* buff2 = nullptr;
            if (!buff2)
                buff2 = bf.Get();

            bool ret = true;

            EasyPacket packet(buff);
            SessionID_t session_id = *packet.SessionID();

            PeerList::iterator cache_peer = peer_cache.find(session_id);
            if (cache_peer == peer_cache.end())
            {
                host.session_id = session_id;
                host.lastReceive = std::chrono::high_resolution_clock::now();
                ret = ReadKeyFromMYSQL(*packet.SessionID(), host.secret.first);
            }
            else
            {
                bool alive = cache_peer->second.alive();
                if ((session_id == cache_peer->second.session_id) && alive)
                {
                    if (host.addr != cache_peer->second.addr)
                    {
                        std::cout << "Peer cache, reconnected with different addr\n";

                        cache_peer->second.addr = host.addr;
                    }
                    else
                    {
                        std::cout << "Peer cache, existing session! Received sequence: " << host.sequence_id_in << ", Peer sequence: " << cache_peer->second.sequence_id_in << "\n";
                    }
                    cache_peer->second.lastReceive = std::chrono::high_resolution_clock::now();
                }
                else
                {
                    std::cout << "Peer cache, peer dead2\n";
                    if (!alive)
                        peer_cache.erase(cache_peer);
                    ret = false;
                }
            }            
            
            if (ret && cache_peer == peer_cache.end())
                ret = EasyPacket::MakeDecrypted(host, buff);
            else if(ret && cache_peer != peer_cache.end())
                ret = EasyPacket::MakeDecrypted(cache_peer->second, buff);

            if (ret)
                ret = EasyPacket::MakeDecompressed(buff, buff2);

            if (ret && cache_peer == peer_cache.end())
                ret = MakeDeserialized(buff2, host.receive);
            else if (ret && cache_peer != peer_cache.end())
                ret = MakeDeserialized(buff2, cache_peer->second.receive);

            if(ret && cache_peer == peer_cache.end())
                peer_cache.insert({ session_id, host });

            return ret;
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

    int ClientResetSendSequenceCounter(lua_State* L)
    {
        client.sequence_id_out= 0U;
        return 0;
    }

    int ClientResetReceiveSequenceCounter(lua_State* L)
    {
        client.sequence_id_in = 0U;
        return 0;
    }

    int ClientSend(lua_State* L)
    {
        EasyBuffer* buff = bf.Get();
        EasyBuffer* buff2 = bf.Get();
        if (buff && buff2)
        {
            // Sample Payload
            std::vector<EasyNetObj*> objs;
            objs.push_back(new pHello("Hi! I'm client."));

            // Prepare Packet
            bool result = true;

            if (result)
                result = MakeSerialized(buff, objs);

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

            std::vector<EasyNetObj*> objs;

            bool status = (res == WSAEISCONN);

            if (status)
                status = EasyPacket::MakeDecrypted(client, buff);

            if (status)
                status = EasyPacket::MakeDecompressed(buff, buff2);

            if (status)
                status = MakeDeserialized(buff2, objs);

            if (status)
            {
                for (EasyNetObj* o : objs)
                    if (o->packetID == HELLO)
                        std::cout << "Client Received Hello Packet. Message: " << (*(pHello*)o).message << "\n";
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
        ClientSend(L);
        std::this_thread::sleep_for(std::chrono::milliseconds(1500U));
        ClientReceive(L);
        return 0;
    }

    int ClientSR(lua_State* L)
    {
        ClientSend(L);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000U));
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
    lua_register(L, "S", ClientTest::ClientSend);
    lua_register(L, "R", ClientTest::ClientReceive);
    lua_register(L, "B", ClientTest::ClientBoth);
    lua_register(L, "SR", ClientTest::ClientSR);
    lua_register(L, "RSC", ClientTest::ClientResetSendSequenceCounter);
    lua_register(L, "RRC", ClientTest::ClientResetReceiveSequenceCounter);

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
            std::chrono::high_resolution_clock::time_point currentTime = std::chrono::high_resolution_clock::now();
            std::chrono::high_resolution_clock::time_point lastTime = std::chrono::high_resolution_clock::now();

            const double fps_constant = 1000.0 / 144.0;
            const double ups_constant = 1000.0 / 24.0;

            double fps_timer = 0.0;
            double ups_timer = 0.0;

            bool success = true;
            while (success)
            {
                currentTime = std::chrono::high_resolution_clock::now();
                double elapsed_ms = std::chrono::duration<double, std::milli>(currentTime - lastTime).count();
                lastTime = currentTime;

                fps_timer += elapsed_ms;
                ups_timer += elapsed_ms;

                if (fps_timer >= fps_constant)
                {
                    playground.StartRender(fps_timer / 1000.0);
                    success &= playground.Render(fps_timer / 1000.0);
                    playground.ImGUIRender();
                    playground.EndRender();
                    fps_timer = 0.0;
                }

                if (ups_timer >= ups_constant)
                {
                    success &= playground.Update(ups_timer / 1000.0);
                    ups_timer = 0.0;
                }

                success &= !display.ShouldClose();
            }

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

