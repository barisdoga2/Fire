#include "Config.hpp"

#include <iostream>
#include <chrono>

#include <EasyUtils.hpp>
#include <EasyDisplay.hpp>
#include <EasyBuffer.hpp>
#include "EasyPlayground.hpp"

EasyBufferManager* bf = new EasyBufferManager(50U, 1472U);

#ifdef SUBSYSTEM_WINDOWS
#include <windows.h>
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
#else
int main(void)
#endif
{
    EasyPlayground playground{};

    EasyUtils_Init();

    bool running = true;
    if (running)
        running = EasyDisplay::Init({ 1536 * 0.8,864 * 0.8 });

    if (running)
        running = playground.Init(bf);

    if (running)
    {
        using MainClock = std::chrono::high_resolution_clock;

        double fps_timer = 0.0, ups_timer = 0.0;
        MainClock::time_point lastTime{};
        while (running)
        {
            MainClock::time_point currentTime = std::chrono::high_resolution_clock::now();
            double elapsed_ms = std::chrono::duration<double, std::milli>(currentTime - lastTime).count();
            lastTime = currentTime;

            fps_timer += elapsed_ms;
            ups_timer += elapsed_ms;

            if (fps_timer >= FPS)
            {
                playground.StartRender(fps_timer / 1000.0);
                running &= playground.Render(fps_timer / 1000.0);
                playground.EndRender(fps_timer / 1000.0);
                fps_timer = 0.0;
            }

            if (ups_timer >= UPS)
            {
                running &= playground.Update(ups_timer / 1000.0);
                ups_timer = 0.0;
            }

            running &= !EasyDisplay::ShouldClose();
        }
    }
    
	return 0;
}

