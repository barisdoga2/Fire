#pragma once

#include <vector>
#include <string>
#include <glm/glm.hpp>
#include "EasyIO.hpp"

struct GLFWwindow;
struct GLFWmonitor;
class DropData {
public:
	int count{};
	const char** paths{};
};

class ErrorData {
public:
	int error{};
	const char* description{};
};

class MonitorData {
public:
	GLFWmonitor* monitor{};
	int event{};
};

class WindowData {
public:
	GLFWwindow* window{};
	
	glm::ivec2 windowPosition{};
	glm::ivec2 windowSize{};

	bool close{};
	bool refresh{};
	int focused{};
	int iconified{};
	int maximized{};
	bool exitRequested{};

	glm::ivec2 frameBufferSize{};
	glm::fvec2 windowContentScale{};
	glm::fvec2 monitorContentScale{};

	DropData drop{};
	MonitorData monitor{};
	ErrorData error{};

	double fps{}, ups{};
};

class WindowListener : public EasyListener {
public:
	virtual inline bool window_pos_callback(const WindowData& data) { return false; }
	virtual inline bool window_size_callback(const WindowData& data) { return false; }
	virtual inline bool window_close_callback(const WindowData& data) { return false; }
	virtual inline bool window_refresh_callback(const WindowData& data) { return false; }
	virtual inline bool window_focus_callback(const WindowData& data) { return false; }
	virtual inline bool window_iconify_callback(const WindowData& data) { return false; }
	virtual inline bool window_maximize_callback(const WindowData& data) { return false; }
	virtual inline bool window_framebuffer_size_callback(const WindowData& data) { return false; }
	virtual inline bool window_content_scale_callback(const WindowData& data) { return false; }
	virtual inline bool window_drop_callback(const WindowData& data) { return false; }
	virtual inline bool monitor_callback(const WindowData& data) { return false; }
	virtual inline bool error_callback(const WindowData& data) { return false; }
};

class EasyDisplay {
private:
	EasyDisplay() = delete;
	EasyDisplay(EasyDisplay&) = delete;
	
	static bool isInit;
	static std::vector<WindowListener*> listeners;
	static WindowData data;
public:
    static bool Init(glm::tvec2<int> windowSize = { 800, 600 }, glm::tvec2<int> position = { 0, 0 });
	static void DeInit();

	static void Update(double _dt);

	static glm::ivec2 GetWindowSize();
	static void AddListener(WindowListener* listener);
	static bool ShouldClose();
	static float GetAspectRatio();
	static void SetExitRequested(bool requested);
	static void SetTitle(std::string title);
	static void Render(double _dt);
	static GLFWwindow* GetWindow();
	static double RtUPS();
	static double RtFPS();

private:
	static void window_pos_callback(GLFWwindow* window, int x, int y);
	static void window_size_callback(GLFWwindow* window, int width, int height);
	static void window_close_callback(GLFWwindow* window);
	static void window_refresh_callback(GLFWwindow* window);
	static void window_focus_callback(GLFWwindow* window, int focused);
	static void window_iconify_callback(GLFWwindow* window, int iconified);
	static void window_maximize_callback(GLFWwindow* window, int maximized);
	static void window_framebuffer_size_callback(GLFWwindow* window, int width, int height);
	static void window_content_scale_callback(GLFWwindow* window, float xscale, float yscale);
	static void window_drop_callback(GLFWwindow* window, int count, const char** paths);
	static void monitor_callback(GLFWmonitor* monitor, int event);
	static void error_callback(int error, const char* description);
};
