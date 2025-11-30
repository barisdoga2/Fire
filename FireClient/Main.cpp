#include <iostream>
#include <chrono>

#include "WinUtils.hpp"
#include "EasyPlayground.hpp"


int main(int argc, char* argv[])
{
    bool running{};

    WinUtils::Init();

    if (EasyDisplay display({ 1536 * 0.8,864 * 0.8 }); display.Init())
    {
        if (EasyPlayground playground(display); playground.Init())
        {
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
	return 0;
}

