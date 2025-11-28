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
#include <conio.h>
#include <lua.hpp>

#include "Net.hpp"
#include "Server.hpp"

bool running{};
bool stop{};

int Stop(lua_State* L)
{
    stop = true;
    return 0;
}

void LUAListen()
{
    std::cout << "LUA commander is starting..." << std::endl;

    lua_State* L;

    L = luaL_newstate();
    luaL_openlibs(L);
    lua_register(L, "ServerStats", Server::Stats);
    lua_register(L, "Q", Server::Stats);
    lua_register(L, "Stop", Stop);
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
        if (_kbhit())
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

EasyBufferManager bf(50U, 1472U);
Server* server = new Server(&bf, SERVER_PORT);

int main()
{
    if (server->Start())
    {
        if (running = true; running)
        {
            std::thread luaThread = std::thread([&]() { LUAListen(); });
            while (running)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1000U));
                running &= (true && !stop);
            }
            server->Stop();
            luaThread.join();
        }
    }
	return 0;
}