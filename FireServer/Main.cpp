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
#include <chrono>
#include <EasyDisplay.hpp>

#include "ServerNet.hpp"
#include "FireServer.hpp"
#include "ServerUI.hpp"

EasyBufferManager* bm = new EasyBufferManager(50U, 1472U);
FireServer* server = new FireServer();

bool running{};
bool stop{};

#include <windows.h>
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    ServerUI::ForwardStandartIO();

    if (bool running = server->Start(bm); running)
    {
        if (EasyDisplay display({ 800, 600 }, { 1,2 }); display.Init())
        {
            if (ServerUI serverUI(&display, bm, server); serverUI.Init())
            {
                std::chrono::high_resolution_clock::time_point currentTime = std::chrono::high_resolution_clock::now();
                std::chrono::high_resolution_clock::time_point lastTime = std::chrono::high_resolution_clock::now();

                const double fps_constant = 1000.0 / 24.0;
                const double ups_constant = 1000.0 / 10.0;

                double fps_timer = 0.0;
                double ups_timer = 0.0;

                while (running)
                {
                    currentTime = std::chrono::high_resolution_clock::now();
                    double elapsed_ms = std::chrono::duration<double, std::milli>(currentTime - lastTime).count();
                    lastTime = currentTime;

                    fps_timer += elapsed_ms;
                    ups_timer += elapsed_ms;

                    if (ups_timer >= ups_constant)
                    {
                        server->Update(ups_timer / 1000.0);
                        running &= serverUI.Update(ups_timer / 1000.0);
                        ups_timer = 0.0;
                    }

                    if (fps_timer >= fps_constant)
                    {
                        serverUI.StartRender(fps_timer / 1000.0);
                        running &= serverUI.Render(fps_timer / 1000.0);
                        serverUI.EndRender();
                        fps_timer = 0.0;
                    }

                    running &= !display.ShouldClose();
                }
            }
        }
    }

    server->Stop();

    return 0;
}
