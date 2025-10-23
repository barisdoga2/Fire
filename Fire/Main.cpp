/* ########################### TODO ###########################
* 
* Session Management
* Addr Checks
* Session ID Checks
* Sequence ID Checks
* Key Expiry
* Alive Checks
* EasySerializeable Leak Check
* Advanced Resource Sharing and Threading
* Server Graphical UI
* Handshake/Ack System
* Packet Loss Handling
* Ping Measurement System
* Character Reading/Saving using Database
* Simple World Share
* Simple World Sync
* Simple Movement 
*  
* ########################################################## */
#include <iostream>
#include <chrono>
#include <conio.h>

#include <lua.hpp>

#include "ClientTest.hpp"
#include "WinUtils.hpp"

//#define REMOTE
//#define CLIENT
#define SERVER
#define SERVER_PORT 54000U



#ifdef SERVER
#include "Server.hpp"
#endif

#ifdef REMOTE
#define SERVER_IP "31.210.43.142"
#else
#define SERVER_IP "127.0.0.1"
#endif

#ifdef CLIENT
#include "EasyPlayground.hpp"
#endif

#include "Sched.hpp"
EasyBufferManager bf(50U, 1472U);
ClientTest client(bf, SERVER_IP, SERVER_PORT);

int ClientWebRequest(lua_State* L) { return client.ClientWebRequest(); }
int ClientResetSendSequenceCounter(lua_State* L) { return client.ClientResetSendSequenceCounter(); }
int ClientResetReceiveSequenceCounter(lua_State* L) { return client.ClientResetReceiveSequenceCounter(); }
int ClientSend(lua_State* L) { return client.ClientSend(); }
int ClientReceive(lua_State* L) { return client.ClientReceive(); }
int ClientBoth(lua_State* L) { return client.ClientBoth(); }
int ClientSR(lua_State* L) { return client.ClientSR(); }
int BufferManagerStatistics(lua_State* L){std::cout << bf.Stats();return 0;}

bool running = false;
void LUAListen()
{
    std::cout << "LUA commander is starting..." << std::endl;

    lua_State* L;

    L = luaL_newstate();
    luaL_openlibs(L);
    lua_register(L, "W", ClientWebRequest);
    lua_register(L, "S", ClientSend);
    lua_register(L, "R", ClientReceive);
    lua_register(L, "B", ClientBoth);
    lua_register(L, "SR", ClientSR);
    lua_register(L, "RSC", ClientResetSendSequenceCounter);
    lua_register(L, "RRC", ClientResetReceiveSequenceCounter);
    lua_register(L, "BF", BufferManagerStatistics);
#ifdef SERVER
    lua_register(L, "ServerStats", Server::Stats);
    lua_register(L, "Q", Server::Stats);
#endif
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
            int c = _getch();
            if ((char)c == '\r' || (char)c == '\n') {
                if (input2 == "exit") {
                    running = false;
                    break;
                }
                input = input2;
                std::cout << std::endl;
                input2.clear();
            }
            else if ((char)c == '\b') {
                if (!input2.empty()) {
                    input2.pop_back();
                    std::cout << "\b \b";
                }
            }
            else {
                input2.push_back((char)c);
                std::cout << (char)c;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

    }

    std::cout << "LUA commander is closing..." << std::endl;

    lua_close(L);
}

int main(int argc, char* argv[])
{
    EasySchedTest();
    bool isServer = true;
    if (argc > 1) 
    {
        std::string arg1 = argv[1];
        if (arg1 == "no_server") 
        {
            isServer = false;
        }
        else 
        {
            std::cout << "Unknown argument: " << arg1 << "\n";
        }
    }

    WinUtils::Init();

    std::thread luaThread;

#ifdef SERVER
    Server* server = nullptr;
    if (isServer)
    {
        Server* server = new Server(&bf, SERVER_PORT);
        if (server->Start())
        {
            if (running = server->IsRunning(); running)
            {
                luaThread = std::thread([&]() { LUAListen(); });
#ifndef CLIENT
                while (running)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1000U));
                    if (running)
                        running = server->IsRunning();
                }
#endif
            }
        }
    }
    else
    {
        running = true;
        luaThread = std::thread([&]() { LUAListen(); });
        while (running)
            std::this_thread::sleep_for(std::chrono::milliseconds(1000U));
    }
#elif defined(CLIENT)
    if (EasyDisplay display({ 1024,768 }); display.Init())
    {
        if (EasyPlayground playground(display); playground.Init())
        {
            luaThread = std::thread([&]() { LUAListen(); });

            std::chrono::high_resolution_clock::time_point currentTime = std::chrono::high_resolution_clock::now();
            std::chrono::high_resolution_clock::time_point lastTime = std::chrono::high_resolution_clock::now();

            const double fps_constant = 1000.0 / 144.0;
            const double ups_constant = 1000.0 / 24.0;

            double fps_timer = 0.0;
            double ups_timer = 0.0;

            running = true;
            while (running)
            {
                currentTime = std::chrono::high_resolution_clock::now();
                double elapsed_ms = std::chrono::duration<double, std::milli>(currentTime - lastTime).count();
                lastTime = currentTime;

                fps_timer += elapsed_ms;
                ups_timer += elapsed_ms;

                if (fps_timer >= fps_constant)
                {
                    playground.StartRender(fps_timer / 1000.0);
                    running &= playground.Render(fps_timer / 1000.0);
                    playground.ImGUIRender();
                    playground.EndRender();
                    fps_timer = 0.0;
                }

                if (ups_timer >= ups_constant)
                {
                    running &= playground.Update(ups_timer / 1000.0);
                    ups_timer = 0.0;
                }

                running &= !display.ShouldClose();
            }
        }
    }
#else
    running = true;
    luaThread = std::thread([&]() { LUAListen(); });
    while (running)
        std::this_thread::sleep_for(std::chrono::milliseconds(1000U));
#endif

#ifdef SERVER
    delete server;
#endif

    if (luaThread.joinable())
        luaThread.join();

	return 0;
}

