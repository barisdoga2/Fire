
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

EasyBufferManager bf(50U, 1472U);
ClientTest client(bf, SERVER_IP, SERVER_PORT);


int BufferManagerStatistics(lua_State* L)
{
    std::cout << bf.Stats()<<"\n";
    return 0;
}

void LUAListen(bool& running)
{
    std::cout << "LUA commander is starting..." << std::endl;

    lua_State* L;

    L = luaL_newstate();
    luaL_openlibs(L);
    lua_register(L, "W", ClientTest::ClientWebRequestS);
    lua_register(L, "S", ClientTest::ClientSendS);
    lua_register(L, "R", ClientTest::ClientReceiveS);
    lua_register(L, "B", ClientTest::ClientBothS);
    lua_register(L, "SR", ClientTest::ClientSRS);
    lua_register(L, "RSC", ClientTest::ClientResetSendSequenceCounterS);
    lua_register(L, "RRC", ClientTest::ClientResetReceiveSequenceCounterS);
    lua_register(L, "BF", BufferManagerStatistics);

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

int main()
{
    WinUtils::Init();

    bool luaRunning = true;
    std::thread luaThread = std::thread([&]() { LUAListen(luaRunning); });

#ifdef SERVER
    Server* server = new Server(&bf, SERVER_PORT);
    if (server->Start())
    {
#ifndef CLIENT
        while (luaRunning && server->running)
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

#if !defined(CLIENT) || !defined(SERVER)
    while (luaRunning)
        std::this_thread::sleep_for(std::chrono::milliseconds(10000U));
#endif

    luaRunning = false;

#ifdef SERVER
    delete server;
#endif

    if (luaThread.joinable())
        luaThread.join();

	return 0;
}

