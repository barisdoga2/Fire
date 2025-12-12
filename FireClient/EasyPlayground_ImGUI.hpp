#include "pch.h"

#include "EasyPlayground.hpp"

#include <EasyUtils.hpp>
#include <EasySerializer.hpp>
#include "../FireServer/ServerNet.hpp"
#include <EasySocket.hpp>
#include <imgui.h>
#include <imgui_internal.h>
#include "../3rd_party/imgui_impl_opengl3.h"
#include "../3rd_party/imgui_impl_glfw.h"
#include <glad/glad.h>
#include <stb_image.h>
#include <string>
#include <chrono>

#include "ClientNetwork.hpp"

static bool isConsoleWindow = true;

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

inline ImTextureID LoadTextureSTB(const char* filename, int* outW = nullptr, int* outH = nullptr)
{
    int w, h, ch;
    unsigned char* data = stbi_load(filename, &w, &h, &ch, 4);
    if (!data) return 0;

    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);

    if (outW) *outW = w;
    if (outH) *outH = h;

    return (ImTextureID)(intptr_t)texID;
}

class ChatMessage {
public:
    static inline std::vector<ChatMessage> messages{};
    static inline bool scrollToBottom{};
    static inline bool isBroadcastMessage{};
    static inline std::string broadcastMessage{};

    std::string username;
    std::string message;
    unsigned long long timeSent = 0;
    unsigned long long timeout = 10000000000;

    ChatMessage(const std::string& m, const std::string& u,  unsigned long long ts) : username(u), message(m), timeSent(ts)
    { }

    std::string toString() const
    {
        return username + ": " + message;
    }

    bool expired(unsigned long long now) const
    {
        return (now - timeSent) >= timeout;
    }

    static void OnChatMessageReceived(const std::string& msg,const std::string& user,unsigned long long timeSent)
    {
        messages.emplace_back(user, msg, timeSent);
        scrollToBottom = true;
    }

    static void Update(unsigned long long now)
    {
        for (auto it = messages.begin(); it != messages.end(); )
        {
            if (it->expired(now))
                it = messages.erase(it);
            else
                ++it;
        }
    }
};

void EasyPlayground::ImGUI_DrawConsoleWindow()
{
    if (!isConsoleWindow)
        return;

    using namespace UIConsole;

    UIConsole::ImGuiConsoleBuf::PruneOldLogs();

    const float W = (float)display->windowSize.x;
    const float H = (float)display->windowSize.y;

    ImGui::SetNextWindowPos(ImVec2(0.0f, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(W * 0.55f, H * 0.35f), ImGuiCond_Always);

    ImGui::Begin("Console", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

    // Scroll
    {
        ImGui::BeginChild("console_scroll", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() - 5), false, ImGuiWindowFlags_HorizontalScrollbar);

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
            // Process 'gConsoleInput'
            gConsoleLines.push_back({
    TimeNow_HHMMSS() + " - " + gConsoleInput,
    std::chrono::steady_clock::now()
                });
            gConsoleInput[0] = 0;
        }
    }

    ImGui::End();
}

void EasyPlayground::ImGUI_DrawChatWindow()
{
    static char inputBuf[256] = { 0 };

    const ImVec2 winSize(400, 160);

    ImVec2 vp = ImGui::GetMainViewport()->Pos;
    ImVec2 pos(
        vp.x + 10,
        vp.y + ImGui::GetMainViewport()->Size.y - winSize.y - 10
    );

    ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(winSize, ImGuiCond_Always);

    if (!ImGui::Begin("ChatWindow", nullptr,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar))
    {
        ImGui::End();
        return;
    }

    float inputHeight = 26.0f;
    float chatHeight = winSize.y - inputHeight - 22.0f;

    ImGui::BeginChild("ChatScrollRegion",
        ImVec2(0, chatHeight),
        false,
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_AlwaysVerticalScrollbar);

    for (const auto& msg : ChatMessage::messages)
        ImGui::TextWrapped("%s", msg.toString().c_str());

    if (ChatMessage::scrollToBottom)
    {
        ImGui::SetScrollHereY(1.0f);
        ChatMessage::scrollToBottom = false;
    }

    ImGui::EndChild();

    ImGui::PushItemWidth(winSize.x - 90.0f);

    bool send = false;

    if (ImGui::InputText("##ChatInput",
        inputBuf,
        IM_ARRAYSIZE(inputBuf),
        ImGuiInputTextFlags_EnterReturnsTrue))
    {
        send = true;
    }

    ImGui::SameLine();

    if (ImGui::Button("Send", ImVec2(63, inputHeight)))
        send = true;

    if (send)
    {
        std::string text(inputBuf);

        if (!text.empty())
        {
            // Send
            network->GetSendCache().push_back(new sChatMessage(text, "", 0));
            inputBuf[0] = '\0';
        }
    }

    ImGui::End();
}

void EasyPlayground::ImGUI_PlayerInfoWindow()
{
    // Position: TOP RIGHT
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImVec2 pos = ImVec2(vp->WorkPos.x + vp->WorkSize.x - 10,  // right - 10px
        vp->WorkPos.y + 10);                 // top + 10px
    ImGui::SetNextWindowPos(pos, ImGuiCond_Always, ImVec2(1.0f, 0.0f));

    // Auto-fit size horizontally & vertically
    ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(400, 400));

    ImGui::Begin("PlayerInfo",
        nullptr,
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoSavedSettings);

    // ---- Header ----
    ImGui::Text("Username:   %s", network->session.username.c_str());
    ImGui::Text("User ID:    %u", network->session.stats.uid);
    ImGui::Text("Session ID: %u", network->session.sid);
    ImGui::Separator();

    // ---- Stats ----
    ImGui::Text("Gold:       %u", network->session.stats.golds);
    ImGui::Text("Diamond:    %u", network->session.stats.diamonds);
    ImGui::Text("Game Time:  %.1f hours", network->session.stats.gametime);
    ImGui::Text("Champions Owned: %d", (int)network->session.stats.champions_owned.size());

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // ---- Logout Button ----
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.85f, 0.2f, 0.2f, 1));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.95f, 0.25f, 0.25f, 1));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.70f, 0.15f, 0.15f, 1));

    if (ImGui::Button("Logout", ImVec2(120, 28)))
        network->Stop();

    ImGui::PopStyleColor(3);
    ImGui::End();
}

void EasyPlayground::ImGUI_ChampionSelectWindow()
{
	static bool loaded = false;
	static std::vector<ImTextureID> icons;

	if (!loaded)
	{
		icons.push_back(LoadTextureSTB(GetRelPath("res/images/champions/1.png").c_str()));
		icons.push_back(LoadTextureSTB(GetRelPath("res/images/champions/2.png").c_str()));
		icons.push_back(LoadTextureSTB(GetRelPath("res/images/champions/3.png").c_str()));
		icons.push_back(LoadTextureSTB(GetRelPath("res/images/champions/4.png").c_str()));
		icons.push_back(LoadTextureSTB(GetRelPath("res/images/champions/5.png").c_str()));
		loaded = true;
	}

    int N = (int)icons.size();
    int clicked = -1;
    if (N != 0)
    {
        const int cols = 5;
        ImVec2 winSize(420, 252);
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(winSize, ImGuiCond_Always);;
        ImGui::Begin("Champion Select", nullptr, ImGuiWindowFlags_NoResize |ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove |ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings);
        auto CenterItem = [&](float width)
            {
                ImGui::SetCursorPosX((winSize.x - width) * 0.5f);
            };
        auto CenteredText = [&](const char* txt)
            {
                float w = ImGui::CalcTextSize(txt).x;
                ImGui::SetCursorPosX((winSize.x - w) * 0.5f);
                ImGui::Text("%s", txt);
            };
        CenteredText("Champion Select");
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::Spacing();
        for (int i = 0; i < N; i++)
        {
            bool owned = std::find(network->session.stats.champions_owned.begin(), network->session.stats.champions_owned.end(),i + 1) != network->session.stats.champions_owned.end();
            ImGui::PushID(i);
            if (!owned)
            {
                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.40f);
                ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
            }
            bool pressed =ImGui::ImageButton(std::to_string(i).c_str(), icons[i], ImVec2(64.0f, 64.0f));

            if (pressed && owned)
                clicked = i;
            if (!owned)
            {
                ImGui::PopItemFlag();
                ImGui::PopStyleVar();
            }
            ImGui::PopID();
            if ((i % cols) != cols - 1)
                ImGui::SameLine(0, 10.0f);
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::Spacing();

        float btnWidth = 100.0f;
        CenterItem(btnWidth);
        if (ImGui::Button("Logout", ImVec2(120, 28)))
            network->Stop();

        ImGui::End();
    }

    if (clicked >= 0)
    {
        network->ChampionSelect(clicked + 1);
    }
}

void EasyPlayground::ImGUI_LoginStatusWindow()
{
	ImVec2 winSize(420, 252);   // smaller window
	ImVec2 center = ImGui::GetMainViewport()->GetCenter();

	// Center on both X and Y precisely
	ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowSize(winSize, ImGuiCond_Always);

	ImGui::Begin("Login Status", nullptr,
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoSavedSettings);

	auto CenteredText = [&](const char* txt)
		{
			float w = ImGui::CalcTextSize(txt).x;
			ImGui::SetCursorPosX((winSize.x - w) * 0.5f);
			ImGui::Text("%s", txt);
		};

	auto CenterItem = [&](float width)
		{
			ImGui::SetCursorPosX((winSize.x - width) * 0.5f);
		};

	CenteredText("Info");
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();
	ImGui::Spacing();

	// Text
	float textWidth = ImGui::CalcTextSize(network->StatusText().c_str()).x;

	CenterItem(textWidth);
	ImGui::Text("%s", network->StatusText().c_str());

	ImGui::Spacing();
	ImGui::Spacing();

	// Center button
	float btnWidth = 100.0f;
	CenterItem(btnWidth);

	if (!network->IsLoginFailed())
	{
		if (ImGui::Button("Cancel", ImVec2(btnWidth, 28)))
		{
            network->Stop();
		}
	}
	else
	{
		if (ImGui::Button("OK", ImVec2(btnWidth, 28)))
		{
            network->Stop();
		}
	}

	ImGui::End();
}

void EasyPlayground::ImGUI_BroadcastMessageWindow()
{
    if (!ChatMessage::isBroadcastMessage || ChatMessage::broadcastMessage.length() == 0)
        return;

    ImVec2 winSize(420, 252);   // smaller window
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();

    // Center on both X and Y precisely
    ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(winSize, ImGuiCond_Always);

    ImGui::Begin("Server Announcement", nullptr,
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoSavedSettings);

    auto CenteredText = [&](const char* txt)
        {
            float w = ImGui::CalcTextSize(txt).x;
            ImGui::SetCursorPosX((winSize.x - w) * 0.5f);
            ImGui::Text("%s", txt);
        };

    auto CenterItem = [&](float width)
        {
            ImGui::SetCursorPosX((winSize.x - width) * 0.5f);
        };

    CenteredText("Announcement");
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::Spacing();

    // Text
    float textWidth = ImGui::CalcTextSize(ChatMessage::broadcastMessage.c_str()).x;

    CenterItem(textWidth);
    ImGui::Text("%s", ChatMessage::broadcastMessage.c_str());

    ImGui::Spacing();
    ImGui::Spacing();

    // Center button
    float btnWidth = 100.0f;
    CenterItem(btnWidth);
    if (ImGui::Button("OK", ImVec2(btnWidth, 28)))
    {
        ChatMessage::isBroadcastMessage = false;
        ChatMessage::broadcastMessage = "";
    }

    ImGui::End();
}

void EasyPlayground::ImGUI_LoginWindow()
{
	ImVec2 winSize = ImVec2(420, 252);
	ImGui::SetNextWindowSize(winSize, ImGuiCond_Always);
	ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(),
		ImGuiCond_Always, ImVec2(0.5f, 0.5f));

	ImGui::Begin("Login", nullptr,
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoTitleBar);

	auto CenteredText = [&](const char* txt)
		{
			float w = ImGui::CalcTextSize(txt).x;
			ImGui::SetCursorPosX((winSize.x - w) * 0.5f);
			ImGui::Text("%s", txt);
		};

	auto CenteredItem = [&](float itemWidth)
		{
			float x = (winSize.x - itemWidth) * 0.5f;
			ImGui::SetCursorPosX(x);
		};

	CenteredText("Login");
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();
	ImGui::Spacing();

	// Username
    network->session.username.resize(USERNAME_LEGTH, '\0');
	CenteredText("Username");
	CenteredItem(240);
	ImGui::InputText("##username", network->session.username.data(), network->session.username.size());
	ImGui::Spacing();

	// Password
    network->session.password.resize(PASSWORD_LEGTH, '\0');
	CenteredText("Password");
	CenteredItem(240);
	ImGui::InputText("##password", network->session.password.data(), network->session.password.size(), ImGuiInputTextFlags_Password);
	ImGui::Spacing();

	// Remember me
    float w = ImGui::CalcTextSize("Remember me").x + 30.0f;
    CenteredItem(w);
    ImGui::Checkbox("Remember me", &rememberMe);

	ImGui::Spacing();
	ImGui::Spacing();

	// Login button
	float btnWidth = 140;
	CenteredItem(btnWidth);
	if (ImGui::Button("Login", ImVec2(btnWidth, 32)))
	{
		// Save config on click
		if (!network->IsLoggingIn())
		{
			loginThread = std::thread([this]() {network->Login(SERVER_URL); });
			loginThread.detach();
		}
	}

	ImGui::End();
}

void EasyPlayground::ImGUI_DebugWindow()
{
    ImVec2 winSize = ImVec2(340, 140);
    ImGui::SetNextWindowSize(winSize, ImGuiCond_Always);

    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImVec2 pos = ImVec2(
        vp->WorkPos.x + vp->WorkSize.x - winSize.x,
        vp->WorkPos.y + vp->WorkSize.y - winSize.y
    );
    ImGui::SetNextWindowPos(pos, ImGuiCond_Always);

    ImGui::Begin("ImGUI Settings");
    ImGui::Checkbox("Fog Enabled", &imgui_isFog);
    ImGui::Checkbox("Triangles Enabled", &imgui_triangles);
    ImGui::Checkbox("Normals Enabled", &imgui_showNormalLines);
    ImGui::InputFloat("Normal Length", &imgui_showNormalLength);
    ImGui::End();
}


void EasyPlayground::ImGUIRender()
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

    ImGUI_DrawConsoleWindow();

    if(network->IsInGame())
    {
        ImGUI_DebugWindow();
        ImGUI_PlayerInfoWindow();
        ImGUI_BroadcastMessageWindow();
        ImGUI_DrawChatWindow();
    }
    else
    {
        if (network->IsChampionSelect())
        {
            ImGUI_ChampionSelectWindow();
        }
        else if (network->IsLoginFailed() || network->IsLoggingIn())
        {
            ImGUI_LoginStatusWindow();
        }
        else
        {
            ImGUI_LoginWindow();
        }
    }

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
