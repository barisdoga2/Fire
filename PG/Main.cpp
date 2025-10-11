#include <iostream>
#include <chrono>
#include <glm/glm.hpp>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <ozz/animation/runtime/local_to_model_job.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/base/containers/vector.h>
#include <ozz/base/maths/simd_math.h>
#include <ozz/base/maths/math_ex.h>
#include <ozz/base/log.h>
#include <ozz/base/io/stream.h>
#include <ozz/base/io/archive.h>
#include <ozz/options/options.h>
#include <ozz/base/maths/soa_transform.h>

#include "PlaybackController.hpp"
#include "Renderer.hpp"
#include "DisplayManager.hpp"


static inline float GetCurrentMillis()
{
    static long long unsigned int startTimestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    long long unsigned int currentTimestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    return (float)(currentTimestamp - startTimestamp);
}


class Tester {
public: 
    DisplayManager& display;

    Tester(DisplayManager& display) : display(display)
    {

    }

    ~Tester()
    {

    }

    bool Init()
    {
        return true;
    }

    bool Update(float dt)
    {
        return true;
    }

    bool Render(float dt)
    {
        // Early Render
        {
            // Set Viewport
            glViewport(0, 0, display.windowSize.x, display.windowSize.y);
            GL(ClearDepth(1.f));
            GL(ClearColor(.4f, .42f, .38f, 1.f));
            GL(Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

            // Setup default states
            GL(Enable(GL_CULL_FACE));
            GL(CullFace(GL_BACK));
            GL(Enable(GL_DEPTH_TEST));
            GL(DepthMask(GL_TRUE));
            GL(DepthFunc(GL_LEQUAL));
        }

        // Render
        {
           
        }

        // End Render
        {
            // Calculate FPS and Update Title
            float sample = GetCurrentMillis();
            static int frameCtr = 0;
            static float lastSample = sample;
            if (sample - lastSample >= 1000)
            {
                glfwSetWindowTitle(display.window, std::string("FPS: " + std::to_string(frameCtr)).c_str());
                lastSample = sample;
                frameCtr = 0;
            }
            frameCtr++;

            // Update Display and Get Events
            glfwSwapBuffers(display.window);
            glfwPollEvents();
        }

        return true;;
    }

    int Loop()
    {
        static float renderResolution = 1000.f / glfwGetVideoMode(glfwGetPrimaryMonitor())->refreshRate;
        static float updateResolution = 1000.f / 24.f;
        static float renderTimer = 0;
        static float updateTimer = 0;
        static float lastTs = GetCurrentMillis();
        static bool exit_requested = 0;

        exit_requested |= display.ExitRequested();

        if(!exit_requested)
        {
            float currentTs = GetCurrentMillis();
            float deltaTs = currentTs - lastTs;
            lastTs = currentTs;

            renderTimer += deltaTs;
            updateTimer += deltaTs;

            if (renderTimer >= renderResolution)
            {
                exit_requested = Render(renderTimer);
                renderTimer = 0;
            }

            if (updateTimer >= updateResolution)
            {
                exit_requested = Update(updateTimer);
                updateTimer = 0;
            }
        }

        return exit_requested;
    }

    int CleanUp()
    {
        return 0;
    }
};


int main()
{
    int retVal = 0;

    // Display Context
    if (DisplayManager display; display.Init())
    {
        // Tester Context
        if (Tester tester(display); tester.Init())
        {
            do {
                retVal = tester.Loop();
            } while (retVal);
            tester.CleanUp();
        }
        else
        {
            tester.CleanUp();
        }
    }
    else
    {
        display.CleanUp();
    }

	return 0;
}