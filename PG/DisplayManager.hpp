#pragma once

#include <iostream>
#include <glm/glm.hpp>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

class DisplayManager {
public:
    GLFWwindow* window;
    glm::tvec2<int> windowSize = { 800,600 };

    DisplayManager()
    {

    }

    ~DisplayManager()
    {

    }
    void GetMousePos(int* x, int* y)
    {

    }

    int GetMouseWheel() 
    {
        return 0;
    }

    int Init()
    {
        if (!glfwInit())
            return 0;

        // Get Primary Monitor and Get Monitor size
        GLFWmonitor* primary = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(primary);
        glm::tvec2<int> monitorSize = glm::tvec2<int>(mode->width, mode->height);

        // Other Window Hints
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE); // MacOS ?
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        //glfwWindowHint(GLFW_DECORATED, NULL); // Remove the border and titlebar.. 
        //glfwWindowHint(GLFW_SCALE_TO_MONITOR, ???); // DPI Scaling
        glfwWindowHint(GLFW_SAMPLES, 4); // request 4x MSAA

        // Craete The Window
        window = glfwCreateWindow(windowSize.x, windowSize.y, "Hi", NULL, NULL);

        // Check If OK
        if (!window)
        {
            std::cout << "GLFW Window couldn't created" << std::endl;
            glfwTerminate();
            return 0;
        }

        // Center The Window
        glfwSetWindowPos(window, (int)((monitorSize.x - windowSize.x) / 2), 30);

        // Enable GL Context
        glfwMakeContextCurrent(window);

        // VSync
        glfwSwapInterval(false);

        // Get Frame Buffer Size
        //glfwGetFramebufferSize(window, &fboSize.x, &fboSize.y);

        // Final Check
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        {
            std::cout << "Failed to initialize GLAD" << std::endl;
            return 0;
        }

        // Add Resize Link
        //glfwSetWindowSizeCallback(window, DisplayManager::DisplayResized);

        return 1;
    }

    bool ExitRequested()
    {
        return glfwWindowShouldClose(window);
    }

    void CleanUp()
    {

    }
};