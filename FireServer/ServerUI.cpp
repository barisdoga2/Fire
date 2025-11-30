#include "ServerUI.hpp"

#include "GL_Ext.hpp"

#include <iostream>
#include <chrono>
#include <sstream>
#include <sstream>
#include <iomanip>
#include <unordered_map>

#include <glm/gtc/type_ptr.hpp>
#include <GLFW/glfw3.h>
#include <EasyServer.hpp>
#include <imgui.h>
#include <imgui_internal.h>
#include "imgui_impl_opengl3.h"
#include "imgui_impl_glfw.h"
#include "EasyUtility.hpp"

struct SessionDebugInfo
{
	unsigned int id = 0;
	unsigned int userId = 0;
	uint64_t lastReceiveMS = 0;
	std::string ip;

	// Expand with whatever you track
};

class SessionDebugger
{
public:
	std::unordered_map<unsigned int, SessionDebugInfo> sessions;
	unsigned int selectedID = 0;

	// Copy from incoming Session* list
	void UpdateSessions(const std::vector<Session*>& list)
	{
		// Remove old ones not existing anymore:
		std::unordered_set<unsigned int> incoming;

		for (auto* s : list)
		{
			if (!s)
				continue;

			incoming.insert(s->sessionID);

			SessionDebugInfo info{};
			info.id = s->sessionID;
			info.userId = s->userID;
			//info.ip = s->addr;
			info.lastReceiveMS = s->lastReceive.time_since_epoch().count(); // Example

			sessions[info.id] = info;
		}

		// remove stale
		for (auto it = sessions.begin(); it != sessions.end(); )
		{
			if (!incoming.count(it->first))
				it = sessions.erase(it);
			else
				++it;
		}
	}
};

static SessionDebugger gDebugger;


ServerUI::ServerUI(const EasyDisplay& display) : display_(display)
{

}

ServerUI::~ServerUI()
{
	// ImGUI
	{
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}
}

bool ServerUI::Init()
{
	// Callbacks
	static ServerUI* instance = this;
	glfwSetKeyCallback(display_.window, [](GLFWwindow* w, int k, int s, int a, int m) { instance->key_callback(w, k, s, a, m); });
	glfwSetCursorPosCallback(display_.window, [](GLFWwindow* w, double x, double y) { instance->cursor_callback(w, x, y); });
	glfwSetMouseButtonCallback(display_.window, [](GLFWwindow* window, int button, int action, int mods) { instance->mouse_callback(window, button, action, mods); });
	glfwSetScrollCallback(display_.window, [](GLFWwindow* w, double x, double y) { instance->scroll_callback(w, x, y); });
	glfwSetCharCallback(display_.window, [](GLFWwindow* w, unsigned int x) { instance->char_callback(w, x); });

	// ImGUI
	const char* glsl_version = "#version 130";
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

	ImVector<ImWchar> ranges;
	ImFontGlyphRangesBuilder builder;
	builder.AddText(u8"ğĞşŞıİüÜöÖçÇ");
	builder.AddRanges(io.Fonts->GetGlyphRangesDefault());
	builder.BuildRanges(&ranges);
	io.Fonts->AddFontFromFileTTF(GetPath("res/fonts/Arial.ttf").c_str(), 20.f, 0, ranges.Data);
	io.Fonts->Build();
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();
	ImGui_ImplGlfw_InitForOpenGL(display_.window, false);
	ImGui_ImplOpenGL3_Init(glsl_version);

	return true;
}

void ServerUI::OnSessionListUpdated(const std::vector<Session*>& list)
{
	gDebugger.UpdateSessions(list);
}

void ServerUI::ImGUI_DrawSessionDetailWindow()
{
	if (gDebugger.selectedID == 0)
		return;

	auto it = gDebugger.sessions.find(gDebugger.selectedID);
	if (it == gDebugger.sessions.end())
	{
		gDebugger.selectedID = 0;
		return;
	}

	auto& info = it->second;

	ImGui::SetNextWindowSize(ImVec2(420, 250), ImGuiCond_Always);

	if (!ImGui::Begin("Session Details", nullptr,
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoSavedSettings))
	{
		ImGui::End();
		return;
	}

	ImGui::Text("Session ID: %u", info.id);
	ImGui::Separator();
	ImGui::Spacing();

	ImGui::Text("User ID: %u", info.userId);
	ImGui::Text("IP Address: %s", info.ip.c_str());
	ImGui::Text("Last Receive: %llu ms ago", info.lastReceiveMS);

	ImGui::Spacing();
	ImGui::Separator();

	// Close button
	if (ImGui::Button("Close"))
		gDebugger.selectedID = 0;

	ImGui::End();
}


void ServerUI::ImGUI_DrawSessionListWindow()
{
	ImGui::SetNextWindowSize(ImVec2(350, 400), ImGuiCond_Always);

	if (!ImGui::Begin("Sessions", nullptr,
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoSavedSettings))
	{
		ImGui::End();
		return;
	}

	ImGui::Text("Active Sessions: %zu", gDebugger.sessions.size());
	ImGui::Separator();

	for (auto& [id, info] : gDebugger.sessions)
	{
		// Selectable row
		char label[64];
		snprintf(label, sizeof(label), "Session %u", id);

		if (ImGui::Selectable(label, gDebugger.selectedID == id))
		{
			gDebugger.selectedID = id; // open detail window
		}
	}

	ImGui::End();
}


bool ServerUI::Render(double _dt)
{
	bool success = true;

	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	ImGUI_DrawSessionListWindow();
	ImGUI_DrawSessionDetailWindow();

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	return success;
}

bool ServerUI::Update(double _dt)
{
	// UPS Calc
	{
		static double updateTimes[4] = { 0.0 };
		static int frameCount = 0;
		static int index = 0;

		// store current frame delta time
		updateTimes[index] = _dt;
		index = (index + 1) % 4;

		if (frameCount < 4)
			++frameCount;

		// calculate average delta over last N frames
		double avgDt = 0.0;
		for (int i = 0; i < frameCount; ++i)
			avgDt += updateTimes[i];
		avgDt /= frameCount;

		// calculate FPS
		ups = 1.0 / avgDt;
	}


	return !exitRequested;
}

void ServerUI::StartRender(double _dt)
{
	// FPS Calc
	{
		static double frameTimes[25] = { 0.0 };
		static int frameCount = 0;
		static int index = 0;

		// store current frame delta time
		frameTimes[index] = _dt;
		index = (index + 1) % 25;

		if (frameCount < 25)
			++frameCount;

		// calculate average delta over last N frames
		double avgDt = 0.0;
		for (int i = 0; i < frameCount; ++i)
			avgDt += frameTimes[i];
		avgDt /= frameCount;

		// calculate FPS
		fps = 1.0 / avgDt;
	}

	glViewport(0, 0, display_.windowSize.x, display_.windowSize.y);
	GL(ClearDepth(1.f));
	GL(ClearColor(0.5f, 0.7f, 1.0f, 1));
	GL(Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

	// Setup default states
	GL(Disable(GL_CULL_FACE)); //GL(Enable(GL_CULL_FACE));
	GL(CullFace(GL_BACK));
	GL(Enable(GL_DEPTH_TEST));
	GL(DepthMask(GL_TRUE));
	GL(DepthFunc(GL_LEQUAL));
	GL(Enable(GL_MULTISAMPLE));
}

void ServerUI::EndRender()
{
	glfwSetWindowTitle(display_.window, (std::ostringstream() << std::fixed << std::setprecision(3) << "FPS: " << fps << " | UPS: " << ups).str().c_str());
	glfwSwapBuffers(display_.window);
	glfwPollEvents();
}

void ServerUI::scroll_callback(GLFWwindow* window, double xpos, double ypos)
{
	ImGui_ImplGlfw_ScrollCallback(window, xpos, ypos);
	if (ImGui::GetIO().WantCaptureMouse)
		return;
}

void ServerUI::cursor_callback(GLFWwindow* window, double xpos, double ypos)
{
	ImGui_ImplGlfw_CursorPosCallback(window, xpos, ypos);
	if (ImGui::GetIO().WantCaptureMouse)
		return;
}

void ServerUI::mouse_callback(GLFWwindow* window, int button, int action, int mods)
{
	ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
	if (ImGui::GetIO().WantCaptureMouse)
		return;
}

void ServerUI::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
	if (ImGui::GetIO().WantCaptureKeyboard)
		return;
}

void ServerUI::char_callback(GLFWwindow* window, unsigned int codepoint)
{
	ImGui_ImplGlfw_CharCallback(window, codepoint);
	if (ImGui::GetIO().WantCaptureKeyboard)
		return;
}