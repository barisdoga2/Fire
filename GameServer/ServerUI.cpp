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
#include <../3rd_party/imgui_impl_opengl3.h>
#include <../3rd_party/imgui_impl_glfw.h>
#include <EasyDisplay.hpp>
#include <EasySocket.hpp>
#include <EasyUtils.hpp>
#include "GameServer.hpp"

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

		EasyBufferManager::BMStats bmStats{};
		unsigned long long total_networkObjectCreates{}, total_networkObjectDeletes{};
	};
	static UISnapshot gSnapshot;
	static double gSnapshotTimer = 0.0;
}

namespace UIConsole {
	struct UIConsoleLine
	{
		std::string text;
		std::chrono::steady_clock::time_point time;

		UIConsoleLine(std::string text, std::chrono::steady_clock::time_point time) : text(text), time(time)
		{

		}

	};

	static std::vector<UIConsoleLine> gConsoleLines;
	static char gConsoleInput[512] = {};
	static bool gAutoScroll = true;

	class ImGuiConsoleBuf : public std::streambuf
	{
		std::vector<UIConsoleLine>& lines;
		std::string currentLine;
		std::mutex mtx;

	public:
		ImGuiConsoleBuf(std::vector<UIConsoleLine>& lines)
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
					lines.push_back({
	TimeNow_HHMMSS() + " - " + currentLine,
	std::chrono::steady_clock::now()
						});
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
					lines.push_back({
	TimeNow_HHMMSS() + " - " + currentLine,
	std::chrono::steady_clock::now()
						});
					currentLine.clear();
				}
				else
				{
					currentLine.push_back(c);
				}
			}
			return count;
		}

	public:
		static void PruneOldLogs()
		{
			return;
			using namespace std::chrono;

			const auto now = steady_clock::now();
			const auto ttl = seconds(30);

			while (!gConsoleLines.empty())
			{
				if (now - gConsoleLines.front().time > ttl)
					gConsoleLines.erase(gConsoleLines.begin());
				else
					break;
			}
		}

	};

	static std::streambuf* gOldCoutBuf = nullptr;
	static ImGuiConsoleBuf* gImGuiCoutBuf = nullptr;
}

ServerUI::ServerUI(EasyBufferManager* bm, GameServer* server) : bm(bm), server(server)
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
	// Inputs
	{
		EasyIO::Init();

		EasyKeyboard::AddListener(this);
		EasyMouse::AddListener(this);
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
		ImGui_ImplGlfw_InitForOpenGL(EasyDisplay::GetWindow(), false);
		ImGui_ImplOpenGL3_Init(glsl_version);
	}

	UISnapshot::gSnapshotTimer += snapshotPeriod;

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
		server->BroadcastMessage(text);
	}
	else if (type == SHUTDOWN_COMMAND)
	{
		server->Stop(text);
		UISnapshot::gSnapshotTimer += snapshotPeriod;
	}
	else if (type == START_COMMAND)
	{
		server->Start(bm);
		UISnapshot::gSnapshotTimer += snapshotPeriod;
	}
}

void ServerUI::ImGUI_DrawSessionsWindow()
{
	using namespace UISnapshot;

	const float W = (float)EasyDisplay::GetWindowSize().x;
	const float H = (float)EasyDisplay::GetWindowSize().y;

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

	UIConsole::ImGuiConsoleBuf::PruneOldLogs();

	const float W = (float)EasyDisplay::GetWindowSize().x;
	const float H = (float)EasyDisplay::GetWindowSize().y;

	ImGui::SetNextWindowPos(ImVec2(0.0f, H * 0.5f), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(W, H * 0.5f), ImGuiCond_Always);

	ImGui::Begin("Console",nullptr,ImGuiWindowFlags_NoMove |ImGuiWindowFlags_NoResize |ImGuiWindowFlags_NoCollapse);

	// Scroll
	{
		ImGui::BeginChild("console_scroll", ImVec2(0, - ImGui::GetFrameHeightWithSpacing() - 5), false, ImGuiWindowFlags_HorizontalScrollbar);

		for (const auto& line : gConsoleLines)
			ImGui::TextUnformatted(line.text.c_str());

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
			gConsoleLines.push_back({
	TimeNow_HHMMSS() + " - " + gConsoleInput,
	std::chrono::steady_clock::now()
				});
			gConsoleInput[0] = 0;
		}
	}

	ImGui::End();
}

void ServerUI::ImGUI_DrawManagementWindow()
{
	using namespace UISnapshot;

	const float W = (float)EasyDisplay::GetWindowSize().x;
	const float H = (float)EasyDisplay::GetWindowSize().y;

	ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(W * 0.5f, H * 0.5f), ImGuiCond_Always);

	ImGui::Begin("Management",
		nullptr,
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoCollapse);

	ImGui::Text("Status: %s", gSnapshot.running ? "Running" : "Stopped");
	if (gSnapshot.running)
	{
		ImGui::Text("Active Sessions: %zu", gSnapshot.sessions.size());
		ImGui::Text("Uptime: %llu sec", gSnapshot.uptimeSec);

		ImGui::Separator();

		ImGui::Text("Total Network Object Create: %llu", gSnapshot.total_networkObjectCreates);
		ImGui::Text("Total Network Object Delete: %llu", gSnapshot.total_networkObjectDeletes);

		ImGui::Separator();

		ImGui::Text("Buffer Count: %llu", gSnapshot.bmStats.total);
		ImGui::Text("Available Buffer Count: %llu", gSnapshot.bmStats.frees);
		ImGui::Text("Busy Buffer Count: %llu", gSnapshot.bmStats.busys);
		ImGui::Text("Total Buffer Gets: %llu", gSnapshot.bmStats.total_gets);
		ImGui::Text("Total Buffer Frees: %llu", gSnapshot.bmStats.total_frees);

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
	}
	else
	{
		ImGui::Separator();

		if (ImGui::Button("Start Server", ImVec2(120.0f, ImGui::GetFrameHeight())))
		{
			OnCommand(START_COMMAND, "");
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
	{
		using namespace UISnapshot;
		gSnapshotTimer += _dt;
		if (gSnapshotTimer >= snapshotPeriod)
		{
			gSnapshotTimer = 0.0;

			gSnapshot.running = server->IsRunning();

			gSnapshot.total_networkObjectCreates = EasySerializeable::total_creates;
			gSnapshot.total_networkObjectDeletes = EasySerializeable::total_deletes;
			gSnapshot.bmStats = bm->StatsV2();

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

	return !EasyDisplay::ShouldClose();
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

	glViewport(0, 0, EasyDisplay::GetWindowSize().x, EasyDisplay::GetWindowSize().y);
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
	glfwSetWindowTitle(EasyDisplay::GetWindow(), (std::ostringstream() << std::fixed << std::setprecision(3) << "FPS: " << fps << " | UPS: " << ups).str().c_str());
	glfwSwapBuffers(EasyDisplay::GetWindow());
	glfwPollEvents();
}

bool ServerUI::button_callback(const MouseData& data)
{
	ImGui_ImplGlfw_MouseButtonCallback(data.window, data.button.button, data.button.action, data.button.mods);
	if (ImGui::GetIO().WantCaptureMouse)
		return false;

	return false;
}

bool ServerUI::scroll_callback(const MouseData& data)
{
	ImGui_ImplGlfw_ScrollCallback(data.window, data.scroll.now.x, data.scroll.now.y);
	if (ImGui::GetIO().WantCaptureMouse)
		return false;

	return false;
}

bool ServerUI::move_callback(const MouseData& data)
{
	ImGui_ImplGlfw_CursorPosCallback(data.window, data.position.now.x, data.position.now.y);
	if (ImGui::GetIO().WantCaptureMouse)
		return false;

	return false;
}

bool ServerUI::key_callback(const KeyboardData& data)
{
	ImGui_ImplGlfw_KeyCallback(data.window, data.key, data.scancode, data.action, data.mods);
	if (ImGui::GetIO().WantCaptureKeyboard)
		return false;

	return false;
}

bool ServerUI::char_callback(const KeyboardData& data)
{
	ImGui_ImplGlfw_CharCallback(data.window, data.codepoint);
	if (ImGui::GetIO().WantCaptureKeyboard)
		return false;

	return false;
}
