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
#include <chrono>
#include <EasyDisplay.hpp>

#include "Net.hpp"
#include "Server.hpp"
#include "ServerUI.hpp"

EasyBufferManager bf(50U, 1472U);
Server* server = new Server(&bf, SERVER_PORT);

bool running{};
bool stop{};

int Stop(lua_State* L)
{
    stop = true;
    return 0;
}

int Broadcast(lua_State* L)
{
    std::string text;
    if (lua_isstring(L, 1)) 
    {
        text = lua_tostring(L, 1);
    }
    else 
    {
        text = "";
    }
    if (text.length() > 0U)
    {
        server->Broadcast({ new sBroadcastMessage(text) });
    }
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
    lua_register(L, "Broadcast", Broadcast);
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
        std::this_thread::sleep_for(std::chrono::milliseconds(10U));

    }

    std::cout << "LUA commander is closing..." << std::endl;

    lua_close(L);
}

int main(int argc, char* argv[])
{
    bool running{};

    if (server->Start())
    {
        if (EasyDisplay display({ 400, 600 }, { 1,2 }); display.Init())
        {
            if (running = true; running)
            {
                if (ServerUI serverUI(display); serverUI.Init())
                {
                    std::chrono::high_resolution_clock::time_point currentTime = std::chrono::high_resolution_clock::now();
                    std::chrono::high_resolution_clock::time_point lastTime = std::chrono::high_resolution_clock::now();

                    const double fps_constant = 1000.0 / 24.0;
                    const double ups_constant = 1000.0 / 1.0;
                    const double debug_constant = 1000.0 / 1.0;

                    double debug_timer = 0.0;
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
                        debug_timer += elapsed_ms;

                        if (fps_timer >= fps_constant)
                        {
                            serverUI.StartRender(fps_timer / 1000.0);
                            running &= serverUI.Render(fps_timer / 1000.0);
                            serverUI.EndRender();
                            fps_timer = 0.0;
                        }

                        if (ups_timer >= ups_constant)
                        {
                            running &= serverUI.Update(ups_timer / 1000.0);
                            ups_timer = 0.0;
                        }

                        if (debug_timer >= debug_constant)
                        {
                            //if(server->m->sessionsMutex.try_lock())
                            {
                                //std::vector<Session*> sessions;
                                //for (auto& sid : server->m->sessionIDs)
                                //    sessions.push_back(server->m->sessions[sid]);
                                //serverUI.OnSessionListUpdated(sessions);
                                //server->m->sessionsMutex.unlock();

                                debug_timer = 0.0;
                            }
                        }

                        running &= !display.ShouldClose();
                    }
                }
            }
        }
    }

    server->Stop();

    return 0;
}

