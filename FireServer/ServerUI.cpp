#include "ServerUI.hpp"

#include "GL_Ext.hpp"

#include <iostream>
#include <chrono>
#include <sstream>
#include <sstream>
#include <iomanip>
#include <streambuf>
#include <mutex>
#include <unordered_map>

#include <glm/gtc/type_ptr.hpp>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
//#include <EasyServer.hpp>
#include <imgui.h>
#include <imgui_internal.h>
#include "../3rd_party/imgui_impl_opengl3.h"
#include "../3rd_party/imgui_impl_glfw.h"
#include <EasyDisplay.hpp>
#include <EasySocket.hpp>
#include "EasyUtils.hpp"
#include "FireServer.hpp"

namespace UISnapshot {
	struct UISession
	{
		SessionID_t sid;
		std::string username;
		std::string ip;
		std::string status;
		uint64_t lastRecvMsAgo;
		uint64_t keyExpiryMs;
	};

	struct UISnapshot
	{
		bool running = false;
		uint64_t startTime = 0;
		uint64_t uptimeSec = 0;
		std::vector<UISession> sessions;
	};
	static UISnapshot gSnapshot;
	static double gSnapshotTimer = 0.0;
}

namespace UIConsole {
	static std::vector<std::string> gConsoleLines;
	static char gConsoleInput[512] = {};
	static bool gAutoScroll = true;

	class ImGuiConsoleBuf : public std::streambuf
	{
	public:
		ImGuiConsoleBuf(std::vector<std::string>& lines)
			: lines(lines)
		{
		}

	protected:
		int overflow(int c) override
		{
			if (c == EOF)
				return EOF;

			std::lock_guard<std::mutex> lock(mtx);

			if (c == '\n')
			{
				if (!currentLine.empty())
				{
					lines.push_back(
						TimeNow_HHMMSS() + " - " + currentLine
					);
					currentLine.clear();
				}
			}
			else
			{
				currentLine.push_back(static_cast<char>(c));
			}

			return c;
		}

		std::streamsize xsputn(const char* s, std::streamsize count) override
		{
			std::lock_guard<std::mutex> lock(mtx);

			for (std::streamsize i = 0; i < count; ++i)
			{
				char c = s[i];
				if (c == '\n')
				{
					lines.push_back(
						TimeNow_HHMMSS() + " - " + currentLine
					);
					currentLine.clear();
				}
				else
				{
					currentLine.push_back(c);
				}
			}
			return count;
		}

	private:
		std::vector<std::string>& lines;
		std::string currentLine;
		std::mutex mtx;
	};

	static std::streambuf* gOldCoutBuf = nullptr;
	static ImGuiConsoleBuf* gImGuiCoutBuf = nullptr;
}

ServerUI::ServerUI(EasyDisplay* display, FireServer* server) : display(display), server(server)
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
	// GLFW Callbacks
	{
		static ServerUI* instance = this;
		glfwSetKeyCallback(display->window, [](GLFWwindow* w, int k, int s, int a, int m) { instance->key_callback(w, k, s, a, m); });
		glfwSetCursorPosCallback(display->window, [](GLFWwindow* w, double x, double y) { instance->cursor_callback(w, x, y); });
		glfwSetMouseButtonCallback(display->window, [](GLFWwindow* window, int button, int action, int mods) { instance->mouse_callback(window, button, action, mods); });
		glfwSetScrollCallback(display->window, [](GLFWwindow* w, double x, double y) { instance->scroll_callback(w, x, y); });
		glfwSetCharCallback(display->window, [](GLFWwindow* w, unsigned int x) { instance->char_callback(w, x); });
	}

	// ImGUI
	{
		const char* glsl_version = "#version 130";
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

		ImVector<ImWchar> ranges;
		ImFontGlyphRangesBuilder builder;
		//builder.AddText(u8"ğĞşŞıİüÜöÖçÇ");
		builder.AddRanges(io.Fonts->GetGlyphRangesDefault());
		builder.BuildRanges(&ranges);
		io.Fonts->AddFontFromFileTTF(GetRelPath("res/fonts/Arial.ttf").c_str(), 14.f, 0, ranges.Data);
		io.Fonts->Build();
		ImGui::StyleColorsDark();
		//ImGui::StyleColorsLight();
		ImGui_ImplGlfw_InitForOpenGL(display->window, false);
		ImGui_ImplOpenGL3_Init(glsl_version);
	}

	return true;
}

void ServerUI::ForwardStandartIO()
{
	using namespace UIConsole;
	gImGuiCoutBuf = new ImGuiConsoleBuf(gConsoleLines);
	gOldCoutBuf = std::cout.rdbuf(gImGuiCoutBuf);
}

void ServerUI::OnCommand(CommandType type, std::string text)
{
	if (type == CONSOLE_COMMAND)
	{
		// ...
	}
	else if (type == BROADCAST_COMMAND)
	{
		server->Broadcast(text);
	}
	else if (type == SHUTDOWN_COMMAND)
	{
		server->Stop(text);
	}
}

void ServerUI::ImGUI_DrawSessionsWindow()
{
	using namespace UISnapshot;

	const float W = (float)display->windowSize.x;
	const float H = (float)display->windowSize.y;

	ImGui::SetNextWindowPos(ImVec2(W * 0.5f, 0.0f), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(W * 0.5f, H * 0.5f), ImGuiCond_Always);

	ImGui::Begin("Sessions",nullptr,ImGuiWindowFlags_NoMove |ImGuiWindowFlags_NoResize |ImGuiWindowFlags_NoCollapse);

	const float colSession = 60.0f;
	const float colUser = 75.0f;
	const float colIP = 110.0f;
	const float colStatus = 60.0f;
	const float colReceive = 100.0f;

	ImGui::Columns(5, nullptr, false);

	ImGui::SetColumnWidth(0, colSession);
	ImGui::SetColumnWidth(1, colUser);
	ImGui::SetColumnWidth(2, colIP);
	ImGui::SetColumnWidth(3, colStatus);
	ImGui::SetColumnWidth(4, colReceive);

	ImGui::Text("  Session");   ImGui::NextColumn();
	ImGui::Text("  Username");  ImGui::NextColumn();
	ImGui::Text("  IP");        ImGui::NextColumn();
	ImGui::Text("  Status");    ImGui::NextColumn();
	ImGui::Text("  Receive");   ImGui::NextColumn();

	ImGui::Separator();
	ImGui::Columns(1);

	// Scroll
	{
		ImGui::BeginChild("sessions_scroll", ImVec2(0, 0), true);

		ImGui::Columns(5, nullptr, false);

		ImGui::SetColumnWidth(0, colSession);
		ImGui::SetColumnWidth(1, colUser);
		ImGui::SetColumnWidth(2, colIP);
		ImGui::SetColumnWidth(3, colStatus);
		ImGui::SetColumnWidth(4, colReceive);

		for (const auto& s : gSnapshot.sessions)
		{
			const double sec = static_cast<double>(s.lastRecvMsAgo) * 0.000000001;

			ImGui::Text("%u", s.sid);                ImGui::NextColumn();
			ImGui::Text("%s", s.username.c_str());   ImGui::NextColumn();
			ImGui::Text("%s", s.ip.c_str());         ImGui::NextColumn();
			ImGui::Text("%s", s.status.c_str());     ImGui::NextColumn();
			ImGui::Text("%.1f s", sec);				 ImGui::NextColumn();
		}

		ImGui::Columns(1);
		ImGui::EndChild();
	}

	ImGui::End();
}

void ServerUI::ImGUI_DrawConsoleWindow()
{
	using namespace UIConsole;

	const float W = (float)display->windowSize.x;
	const float H = (float)display->windowSize.y;

	ImGui::SetNextWindowPos(ImVec2(0.0f, H * 0.5f), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(W, H * 0.5f), ImGuiCond_Always);

	ImGui::Begin("Console",nullptr,ImGuiWindowFlags_NoMove |ImGuiWindowFlags_NoResize |ImGuiWindowFlags_NoCollapse);

	// Scroll
	{
		ImGui::BeginChild("console_scroll", ImVec2(0, - ImGui::GetFrameHeightWithSpacing() - 5), false, ImGuiWindowFlags_HorizontalScrollbar);

		for (const auto& line : gConsoleLines)
			ImGui::TextUnformatted(line.c_str());

		if (gAutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
			ImGui::SetScrollHereY(1.0f);

		ImGui::EndChild();
	}

	ImGui::Separator();

	// Send Button and Area
	{
		const float btnW = 110.0f;
		const float spacing = ImGui::GetStyle().ItemSpacing.x;
		const float inputW = ImGui::GetContentRegionAvail().x - btnW - spacing;
		ImGui::SetNextItemWidth(inputW);
		ImGui::InputText("##cmd", gConsoleInput, sizeof(gConsoleInput));

		ImGui::SameLine();
		if (ImGui::Button("Send", ImVec2(btnW, ImGui::GetFrameHeight())))
		{
			OnCommand(CONSOLE_COMMAND, gConsoleInput);
			gConsoleLines.emplace_back(gConsoleInput);
			gConsoleInput[0] = 0;
		}
	}

	ImGui::End();
}

void ServerUI::ImGUI_DrawManagementWindow()
{
	using namespace UISnapshot;

	const float W = (float)display->windowSize.x;
	const float H = (float)display->windowSize.y;

	ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(W * 0.5f, H * 0.5f), ImGuiCond_Always);

	ImGui::Begin("Management",
		nullptr,
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoCollapse);

	ImGui::Text("Status: %s", gSnapshot.running ? "Running" : "Stopped");
	ImGui::Text("Active Sessions: %zu", gSnapshot.sessions.size());
	ImGui::Text("Uptime: %llu sec", gSnapshot.uptimeSec);

	ImGui::Separator();

	const float btnW = 120.0f;
	const float spacing = ImGui::GetStyle().ItemSpacing.x;
	const float inputW = ImGui::GetContentRegionAvail().x - btnW - spacing;

	static char broadcastMsg[256]{};
	ImGui::SetNextItemWidth(inputW);
	ImGui::InputText("##broadcast", broadcastMsg, sizeof(broadcastMsg));
	ImGui::SameLine();
	if (ImGui::Button("Broadcast Message", ImVec2(btnW, ImGui::GetFrameHeight())))
	{
		OnCommand(BROADCAST_COMMAND, broadcastMsg);
		broadcastMsg[0] = 0;
	}

	ImGui::Separator();

	static char shutdownMsg[256]{};
	ImGui::SetNextItemWidth(inputW);
	ImGui::InputText("##shutdown", shutdownMsg, sizeof(shutdownMsg));
	ImGui::SameLine();
	if (ImGui::Button("Stop Server", ImVec2(btnW, ImGui::GetFrameHeight())))
	{
		OnCommand(SHUTDOWN_COMMAND, shutdownMsg);
		shutdownMsg[0] = 0;
	}

	ImGui::End();
}

bool ServerUI::Render(double _dt)
{
	bool success = true;

	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	ImGUI_DrawSessionsWindow();
	ImGUI_DrawManagementWindow();
	ImGUI_DrawConsoleWindow();

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

	// Snapshot periodically
	const double snapshotPeriod = 2.5;
	{
		using namespace UISnapshot;
		gSnapshotTimer += _dt;
		if (gSnapshotTimer >= snapshotPeriod)
		{
			gSnapshotTimer = 0.0;

			gSnapshot.running = server->IsRunning();

			gSnapshot.sessions.clear();
			uint64_t nowMs = Clock::now().time_since_epoch().count();

			for (auto& [sid, fSession] : server->sessions)
			{
				if (!fSession) continue;

				UISession session{};
				session.sid = sid;
				session.username = fSession->username;
				session.ip = EasySocket::AddrToString(fSession->addr);
				session.status = fSession->logoutRequested ? "Closing" : "Active";
				session.lastRecvMsAgo = nowMs - fSession->recv.time_since_epoch().count();
				session.keyExpiryMs = 0; // fill later if you expose it

				gSnapshot.sessions.push_back(session);
			}
		}
	}

	return !display->ShouldClose();
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

	glViewport(0, 0, display->windowSize.x, display->windowSize.y);
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
	glfwSetWindowTitle(display->window, (std::ostringstream() << std::fixed << std::setprecision(3) << "FPS: " << fps << " | UPS: " << ups).str().c_str());
	glfwSwapBuffers(display->window);
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