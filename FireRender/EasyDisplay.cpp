#include "pch.h"
#include "EasyDisplay.hpp"

#include <iostream>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <vector>





bool EasyDisplay::Init(glm::tvec2<int> sz, glm::tvec2<int> ps)
{
	DeInit();

	if (!glfwInit() || isInit)
		return false;

	data.windowPosition = ps;
	data.windowSize = sz;

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
	data.window = glfwCreateWindow(data.windowSize.x, data.windowSize.y, "Hi", NULL, NULL);

	// Check If OK
	if (!data.window)
	{
		std::cout << "[EasyDisplay] Init - GLFW Window couldn't created" << std::endl;
		glfwTerminate();
	}
	else
	{
		// Center The Window
		glfwSetWindowPos(data.window, (int)((monitorSize.x - data.windowSize.x) / 2), 30);

		if (data.windowPosition.x != 0 && data.windowPosition.y != 0)
			glfwSetWindowPos(data.window, 0, 0 + 30);

		// Enable GL Context
		glfwMakeContextCurrent(data.window);

		// VSync
		glfwSwapInterval(false);

		// Get Frame Buffer Size
		//glfwGetFramebufferSize(data.window, &fboSize.x, &fboSize.y);

		// Final Check
		if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
		{
			std::cout << "[EasyDisplay] Init - Failed to initialize GLAD" << std::endl;

			data = {};
		}
		else
		{
			// Callbacks
			isInit = true;

			glfwGetWindowPos(data.window, &data.windowPosition.x, &data.windowPosition.y);
			glfwGetWindowSize(data.window, &data.windowSize.x, &data.windowSize.y);
			glfwGetWindowContentScale(data.window, &data.windowContentScale.x, &data.windowContentScale.y);
			glfwGetMonitorContentScale(primary, &data.monitorContentScale.x, &data.monitorContentScale.y);
			glfwGetFramebufferSize(data.window, &data.frameBufferSize.x, &data.frameBufferSize.y);

			glfwSetWindowPosCallback(data.window, window_pos_callback);
			glfwSetWindowSizeCallback(data.window, window_size_callback);
			glfwSetWindowCloseCallback(data.window, window_close_callback);
			glfwSetWindowRefreshCallback(data.window, window_refresh_callback);
			glfwSetWindowFocusCallback(data.window, window_focus_callback);
			glfwSetWindowIconifyCallback(data.window, window_iconify_callback);
			glfwSetWindowMaximizeCallback(data.window, window_maximize_callback);
			glfwSetFramebufferSizeCallback(data.window, window_framebuffer_size_callback);
			glfwSetWindowContentScaleCallback(data.window, window_content_scale_callback);
			glfwSetDropCallback(data.window, window_drop_callback);
			glfwSetMonitorCallback(monitor_callback);
			glfwSetErrorCallback(error_callback);
		}
	}

	if(isInit)
		std::cout << "[EasyDisplay] Init - Initialized!" << std::endl;

	return isInit;
}

void EasyDisplay::DeInit()
{
	if (!isInit)
		return;

	glfwSetWindowPosCallback(data.window, nullptr);
	glfwSetWindowSizeCallback(data.window, nullptr);
	glfwSetWindowCloseCallback(data.window, nullptr);
	glfwSetWindowRefreshCallback(data.window, nullptr);
	glfwSetWindowFocusCallback(data.window, nullptr);
	glfwSetWindowIconifyCallback(data.window, nullptr);
	glfwSetWindowMaximizeCallback(data.window, nullptr);
	glfwSetFramebufferSizeCallback(data.window, nullptr);
	glfwSetWindowContentScaleCallback(data.window, nullptr);
	glfwSetDropCallback(data.window, nullptr);
	glfwSetMonitorCallback(nullptr);
	glfwSetErrorCallback(nullptr);

	glfwDestroyWindow(data.window);
	glfwTerminate();

	listeners = {};
	data = {};
	isInit = false;
}

bool EasyDisplay::ShouldClose()
{
	return data.exitRequested || glfwWindowShouldClose(data.window);
}

float EasyDisplay::GetAspectRatio()
{
	return data.windowSize.x / (float)data.windowSize.y;
}

void EasyDisplay::SetExitRequested(bool requested)
{
	data.exitRequested = requested;
}

GLFWwindow* EasyDisplay::GetWindow()
{
	return data.window;
}

glm::ivec2 EasyDisplay::GetWindowSize()
{
	return data.windowSize;
}

void EasyDisplay::AddListener(WindowListener* listener)
{
	listeners.push_back(listener);
}

void EasyDisplay::window_pos_callback(GLFWwindow* window, int x, int y)
{
	data.windowPosition.x = x;
	data.windowPosition.y = y;

	for (WindowListener* listener : listeners)
		if (listener->window_pos_callback(data))
			return;

}

void EasyDisplay::window_size_callback(GLFWwindow* window, int width, int height)
{
	data.windowSize.x = width;
	data.windowSize.y = height;

	for (WindowListener* listener : listeners)
		if (listener->window_size_callback(data))
			return;

}

void EasyDisplay::window_close_callback(GLFWwindow* window)
{
	data.close = true;

	for (WindowListener* listener : listeners)
		if (listener->window_close_callback(data))
			return;

}

void EasyDisplay::window_refresh_callback(GLFWwindow* window)
{
	data.refresh = true;

	for (WindowListener* listener : listeners)
		if (listener->window_refresh_callback(data))
			return;

}

void EasyDisplay::window_focus_callback(GLFWwindow* window, int focused)
{
	data.focused = focused;

	for (WindowListener* listener : listeners)
		if (listener->window_focus_callback(data))
			return;

}

void EasyDisplay::window_iconify_callback(GLFWwindow* window, int iconified)
{
	data.iconified = iconified;

	for (WindowListener* listener : listeners)
		if (listener->window_iconify_callback(data))
			return;

}

void EasyDisplay::window_maximize_callback(GLFWwindow* window, int maximized)
{
	data.maximized = maximized;

	for (WindowListener* listener : listeners)
		if (listener->window_maximize_callback(data))
			return;

}

void EasyDisplay::window_framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	data.frameBufferSize.x = width;
	data.frameBufferSize.y = height;

	for (WindowListener* listener : listeners)
		if (listener->window_framebuffer_size_callback(data))
			return;

}

void EasyDisplay::window_content_scale_callback(GLFWwindow* window, float xscale, float yscale)
{
	data.windowContentScale.x = xscale;
	data.windowContentScale.y = yscale;

	for (WindowListener* listener : listeners)
		if (listener->window_content_scale_callback(data))
			return;

}

void EasyDisplay::window_drop_callback(GLFWwindow* window, int count, const char** paths)
{
	data.drop.count = count;
	data.drop.paths = paths;

	for (WindowListener* listener : listeners)
		if (listener->window_drop_callback(data))
			return;

}

void EasyDisplay::monitor_callback(GLFWmonitor* monitor, int event)
{
	data.monitor.monitor = monitor;
	data.monitor.event = event;

	for (WindowListener* listener : listeners)
		if (listener->monitor_callback(data))
			return;

}

void EasyDisplay::error_callback(int error, const char* description)
{
	data.error.error = error;
	data.error.description = description;

	for (WindowListener* listener : listeners)
		if (listener->error_callback(data))
			return;

}

bool EasyDisplay::isInit = false;
std::vector<WindowListener*> EasyDisplay::listeners = {};
WindowData EasyDisplay::data = {};
