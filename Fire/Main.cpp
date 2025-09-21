
#include <iostream>
#include <box2d/box2d.h>
#include <tinyxml2.h>
#include <ozz/animation/offline/raw_animation.h>
#include <glm/glm.hpp>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <assimp/Importer.hpp>
#include <freetype/freetype.h>
#include <curl/curl.h>
#include <cryptopp/sha.h>

#include "Utils.hpp"

std::string title = "Fire!";
glm::tvec2<int> windowSize = glm::tvec2<int>(800, 600);
GLFWwindow* window;

void Update(float deltaMs)
{

}

void Render(float deltaMs)
{
	// Early Render
	{
		// Set Viewport
		glViewport(0, 0, windowSize.x, windowSize.y);

		// Clear Color
		glClearColor(255, 0, 0, 1);

		// Clear Screen
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	// Actual Render
	{

	}

	// End Render
	{
		// Calculate FPS and Update Title
		float sample = GetCurrentMillis();
		static float frameCtr = 0;
		static float lastSample = sample;
		if (sample - lastSample >= 1000)
		{
			glfwSetWindowTitle(window, std::string(title + " | FPS: " + std::to_string(frameCtr).c_str()).c_str());
			lastSample = sample;
			frameCtr = 0;
		}
		frameCtr++;

		// Update Display and Get Events
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
}

bool CreateDisplay()
{
	if (!glfwInit())
		return false;

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

	// Craete The Window
	window = glfwCreateWindow(windowSize.x, windowSize.y, title.c_str(), NULL, NULL);

	// Check If OK
	if (!window)
	{
		std::cout << "GLFW Window couldn't created" << std::endl;
		glfwTerminate();
		return false;
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
		return false;
	}

	// Add Resize Link
	//glfwSetWindowSizeCallback(window, DisplayManager::DisplayResized);
}

int main()
{
	if (!CreateDisplay())
		return -1;

	// Main Loop
	{
		float renderResolution = 1000.f / glfwGetVideoMode(glfwGetPrimaryMonitor())->refreshRate;
		float updateResolution = 1000.f / 24.f;
		float renderTimer = 0;
		float updateTimer = 0;
		float lastTs = GetCurrentMillis();
		while (!glfwWindowShouldClose(window))
		{
			float currentTs = GetCurrentMillis();
			float deltaTs = currentTs - lastTs;
			lastTs = currentTs;

			renderTimer += deltaTs;
			updateTimer += deltaTs;

			if (renderTimer >= renderResolution)
			{
				Render(renderTimer);
				renderTimer = 0;
			}

			if (updateTimer >= updateResolution)
			{
				Update(updateTimer);
				updateTimer = 0;
			}

		}
	}
	
	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}