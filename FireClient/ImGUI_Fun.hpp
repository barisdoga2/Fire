#include "ClientTest.hpp"
#include "../FireServer/Net.hpp"
#include <EasyServer.hpp>
#include <EasySocket.hpp>

class ChatMessage
{
public:
    std::string username;
    std::string message;
    unsigned long long timeSent = 0;
    unsigned long long timeout = 10000000000;

    ChatMessage(const std::string& m,
        const std::string& u,
        unsigned long long ts)
        : username(u), message(m), timeSent(ts)
    {

    }

    std::string toString() const
    {
        return username + ": " + message;
    }

    bool expired(unsigned long long now) const
    {
        return (now - timeSent) >= timeout;
    }
};

std::vector<ChatMessage> messages;
bool scrollToBottom = false;
char inputBuf[256] = { 0 };

void OnChatMessageReceived(const std::string& msg,const std::string& user,unsigned long long timeSent)
{
    messages.emplace_back(user, msg, timeSent);
    scrollToBottom = true;
}

void UpdateMessageBox(unsigned long long now)
{
    for (auto it = messages.begin(); it != messages.end(); )
    {
        if (it->expired(now))
            it = messages.erase(it);
        else
            ++it;
    }
}


EasyBufferManager bf(50U, 1472U);
ClientTest client(bf, SERVER_IP, SERVER_PORT);

// Common


// Session
std::string username;
SessionID_t sessionID;
uint32_t userID;
PeerCryptInfo* crypt;

// Login
char usernameArr[16] = "";
char passwordArr[32] = "";
bool rememberMe = false;
bool loginInProgress = false;
bool loginStatusWindow = false;
bool loginFailed = false;
bool champSelect = false;
bool loggedIn = false;
std::string loginStatusText;
std::thread loginThread;
std::atomic<bool> loginThreadRunning = false;
bool isRender{};
UserStats stats;
unsigned int championSelected{};
bool isBroadcastMessage{};
std::string broadcastMessage{};

inline bool ParseWebLoginResponse(const std::string& serverUrl,const std::string& username_,const std::string& password)
{
    loginFailed     = false;
    loginStatusText = "Logging in...";

    std::string response = client.ClientWebRequest(serverUrl, username_, password);
    const std::string prefix  = "<textarea name=\"jwt\" readonly>";
    const std::string prefix2 = "<div class=\"msg\">";

    if (response._Starts_with("Curl error:"))
    {
        loginStatusText = response.substr(std::string("Curl error:").length());
        loginFailed     = true;
        return false;
    }

    if (size_t index = response.find(prefix); index != std::string::npos)
    {
        // Got jwt-style payload
        loginStatusText = "Waiting the game server...";

        response = response.substr(index + prefix.length());
        response = response.substr(0, response.find("</textarea>"));

        // sessionID:userID:key
        sessionID = static_cast<SessionID_t>(
            std::stoul(response.substr(0, response.find_first_of(':'))));

        response = response.substr(response.find_first_of(':') + 1U);
        userID   = static_cast<uint32_t>(
            std::stoul(response.substr(0, response.find_first_of(':'))));

        response = response.substr(response.find_first_of(':') + 1U);
        std::string key = response;

        Key_t key_t(KEY_SIZE);
        std::memcpy(key_t.data(), key.data(), KEY_SIZE);

        if (crypt)
            delete crypt;
        crypt   = new PeerCryptInfo(sessionID, 0U, 0U, key_t);
        username = username_;

        if (rememberMe)
            SaveConfig(true, usernameArr, passwordArr);
        else
            SaveConfig(false, "", "");

        return true;
    }

    // Error div
    if (size_t index = response.find(prefix2); index != std::string::npos)
    {
        response       = response.substr(index + prefix2.length());
        response       = response.substr(0, response.find("</div>"));
        loginStatusText = response;
    }
    else
    {
        loginStatusText = "Error while parsing response!";
    }

    loginFailed = true;
    return false;
}

template<typename T, typename Handler>
bool WaitForPacket(std::vector<EasySerializeable*>& recvObjs,std::mutex& recvMtx,unsigned timeoutMs, const char* timeoutMessage,Handler  onPacket)
{
    constexpr unsigned stepMs = 100U;
    unsigned remaining        = timeoutMs / stepMs;

    while (!loginFailed && remaining--)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(stepMs));

        std::lock_guard<std::mutex> lock(recvMtx);
        for (auto it = recvObjs.begin(); it != recvObjs.end(); )
        {
            if (auto* pkt = dynamic_cast<T*>(*it); pkt)
            {
                onPacket(pkt);
                delete pkt;
                recvObjs.erase(it);
                return true;
            }
            else
            {
                ++it;
            }
        }
    }

    if (!loginFailed)
    {
        loginStatusText = timeoutMessage;
        loginFailed     = true;
    }
    return false;
}

inline void Logout()
{
    std::vector<EasySerializeable*> logout = { new sLogoutRequest() };
    if (loggedIn || !loginFailed || champSelect || loginInProgress)
        client.ClientSend(*crypt, logout);

    loggedIn = false;
    loginFailed = true;
    loginInProgress = false;
    loginStatusWindow = false;
    champSelect = false;

    // reset crypt, username, session, etc.
    if (crypt) { delete crypt; crypt = nullptr; }

    sessionID = 0;
    userID = 0;
    username = "";
    stats = {}; // reset
}

inline void LoggedIn(std::vector<EasySerializeable*>& recvObjs, std::mutex& recvMtx, std::vector<EasySerializeable*>& sendObjs, std::mutex& sendMtx)
{
    unsigned heartbeatCtr = 10U;
    std::chrono::steady_clock::time_point lastHeartbeatReceive = Clock::now();
    while (loggedIn)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100U));
        {
            std::lock_guard<std::mutex> lock(recvMtx);
            for (auto it = recvObjs.begin(); it != recvObjs.end(); )
            {
                if (auto* d = dynamic_cast<sDisconnectResponse*>(*it); d)
                {
                    loggedIn = false;
                    loginFailed = true;
                    loginStatusText = d->message;
                    delete d;
                    it = recvObjs.erase(it);
                }
                else if (auto* h = dynamic_cast<sHearbeat*>(*it); h)
                {
                    lastHeartbeatReceive = Clock::now();
                    delete h;
                    it = recvObjs.erase(it);
                }
                else if (auto* b = dynamic_cast<sBroadcastMessage*>(*it); b)
                {
                    isBroadcastMessage = true;
                    broadcastMessage = b->message;
                    delete b;
                    it = recvObjs.erase(it);
                }
                else if (auto* c = dynamic_cast<sChatMessage*>(*it); c)
                {
                    OnChatMessageReceived(c->username, c->message, c->timestamp);
                    delete b;
                    it = recvObjs.erase(it);
                }
                else if (auto* m = dynamic_cast<sPlayerMovementPack*>(*it); m)
                {
                    std::cout << "Movement pack received!\n";
                    delete m;
                    it = recvObjs.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        }

        if (lastHeartbeatReceive + std::chrono::seconds(10) < Clock::now())
        {
            loggedIn = false;
            loginFailed = true;
            loginStatusText = "Disconnect reason: '" + SessionStatus_Str(SERVER_TIMED_OUT) + "'!";
        }
        if (--heartbeatCtr == 0U)
        {
            std::lock_guard<std::mutex> lock(sendMtx);
            sendObjs.push_back(new sHearbeat());
            heartbeatCtr = 10U;
        }

        UpdateMessageBox(Clock::now().time_since_epoch().count());
    }
}

inline void Login(std::string username_, std::string password)
{
	client.client.socket = new EasySocket();

    // Reset state
    loggedIn   = false;
    loginFailed = false;

    if (crypt)
    {
        crypt->key          = {};
        crypt->sequence_id_in  = 0U;
        crypt->sequence_id_out = 0U;
        crypt->session_id      = 0U;
    }

    // 1) Web login + crypt setup
    if (!ParseWebLoginResponse(SERVER_URL, username_, password))
    {
        loginThreadRunning = false;
        isRender           = false;
        return;
    }

    // 2) Prepare shared queues and threads
    std::vector<EasySerializeable*> sendObjs;
    std::vector<EasySerializeable*> recvObjs;
    std::mutex sendMtx, recvMtx;

    std::thread recvTh([&]
    {
        while (!loginFailed)
        {
            if (crypt && recvMtx.try_lock())
            {
                client.ClientReceive(*crypt, recvObjs);
                recvMtx.unlock();
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100U));
        }
    });

    std::thread sendTh([&]
    {
        while (!loginFailed)
        {
            if (crypt && sendMtx.try_lock())
            {
                client.ClientSend(*crypt, sendObjs);
                for (EasySerializeable* s : sendObjs)
                    delete s;
                sendObjs.clear();
                sendMtx.unlock();
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100U));
        }
    });

    // 3) Send initial hello
    {
        std::lock_guard<std::mutex> lock(sendMtx);
        sendObjs.push_back(new sLoginRequest());
    }

    // 4) Wait Login Response
    if (!loginFailed)
    {
        WaitForPacket<sLoginResponse>(
            recvObjs, recvMtx, 3000U, "Login response timeout!",
            [&](sLoginResponse* res)
            {
                loginStatusText = res->message;
                loginFailed     = !res->response;
            });
    }

    // 5) Player boot info
    if (!loginFailed)
    {
        WaitForPacket<sPlayerBootInfo>(
            recvObjs, recvMtx, 3000U, "Player boot timeout!",
            [&](sPlayerBootInfo* info)
            {
                loginStatusText          = "Player boot received!";
                stats.userID             = info->userID;
                stats.gametime           = info->gametime;
                stats.golds              = info->golds;
                stats.diamonds           = info->diamonds;
                stats.tutorial_done      = info->tutorialDone;
                stats.champions_owned    = info->championsOwned;
            });
    }

    // 6) Champion select (wait external UI to set championSelected & clear champSelect)
    if (!loginFailed)
    {
        championSelected = 0U;
        champSelect      = true;

        while (champSelect && !loginFailed)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100U));
            std::lock_guard<std::mutex> lock(recvMtx);
            for (auto it = recvObjs.begin(); it != recvObjs.end(); )
            {
                if (auto* d = dynamic_cast<sDisconnectResponse*>(*it); d)
                {
                    champSelect = false;
                    loggedIn = false;
                    loginFailed = true;
                    loginStatusText = d->message;

                    delete d;
                    it = recvObjs.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        }

        if (!loginFailed)
        {
            loginStatusText = "Selecting champion!";
            std::lock_guard<std::mutex> lock(sendMtx);
            sendObjs.push_back(new sChampionSelectRequest(championSelected));
        }
    }

    // 7) Champion select response
    if (!loginFailed)
    {
        WaitForPacket<sChampionSelectResponse>(
            recvObjs, recvMtx, 3000U, "Champion select response timeout!",
            [&](sChampionSelectResponse* res)
            {
                loginFailed     = !res->response;
                loginStatusText = res->message;
            });
    }

    // 8) Main online loop
    loggedIn = !loginFailed;

    if (loggedIn)
    {
        LoggedIn(recvObjs, recvMtx, sendObjs, sendMtx);
    }

    if (sendTh.joinable()) sendTh.join();
    if (recvTh.joinable()) recvTh.join();

    loginThreadRunning = false;
    isRender           = false;
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

int DrawChampionSelectWindow(const std::vector<ImTextureID>& icons, float buttonSize = 64.0f, float padding = 10.0f)
{
    int N = (int)icons.size();
    if (N == 0)
        return -1;

    const int cols = 5;   // <<< FIXED: ALWAYS 3 PER ROW

    // Window small placeholder size (auto-resize will override)
    ImVec2 winSize(420, 252);   // smaller window
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();

    // Center on both X and Y precisely
    ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(winSize, ImGuiCond_Always);;

   
    ImGui::Begin("Champion Select",
        nullptr,
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


    CenteredText("Champion Select");
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::Spacing();

    int clicked = -1;

    for (int i = 0; i < N; i++)
    {
        bool owned = std::find(
            stats.champions_owned.begin(),
            stats.champions_owned.end(),
            i + 1) != stats.champions_owned.end();

        ImGui::PushID(i);

        if (!owned)
        {
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.40f);
            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        }

        bool pressed =
            ImGui::ImageButton(std::to_string(i).c_str(), icons[i], ImVec2(buttonSize, buttonSize));

        if (pressed && owned)
            clicked = i;

        if (!owned)
        {
            ImGui::PopItemFlag();
            ImGui::PopStyleVar();
        }

        ImGui::PopID();

        // Put 3 items per row
        if ((i % cols) != cols - 1)
            ImGui::SameLine(0, padding);
    }

    ImGui::End();
    return clicked;
}

void EasyPlayground::ImGUI_DrawChatWindow()
{
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

    for (const auto& msg : messages)
        ImGui::TextWrapped("%s", msg.toString().c_str());

    if (scrollToBottom)
    {
        ImGui::SetScrollHereY(1.0f);
        scrollToBottom = false;
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
            std::vector<EasySerializeable*> chatMsg = { new sChatMessage(text, "", 0)};
            client.ClientSend(*crypt, chatMsg);
            inputBuf[0] = '\0';
        }
    }

    ImGui::End();
}

void EasyPlayground::ImGUI_PlayerInfoWindow()
{
    if (!loggedIn && !champSelect)
        return;

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
    ImGui::Text("Username:   %s", username.c_str());
    ImGui::Text("User ID:    %u", userID);
    ImGui::Text("Session ID: %u", sessionID);
    ImGui::Separator();

    // ---- Stats ----
    ImGui::Text("Gold:       %u", stats.golds);
    ImGui::Text("Diamond:    %u", stats.diamonds);
    ImGui::Text("Game Time:  %.1f hours", stats.gametime);
    ImGui::Text("Champions Owned: %d", (int)stats.champions_owned.size());

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // ---- Logout Button ----
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.85f, 0.2f, 0.2f, 1));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.95f, 0.25f, 0.25f, 1));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.70f, 0.15f, 0.15f, 1));

    if (ImGui::Button("Logout", ImVec2(120, 28)))
    {
        Logout();
    }

    ImGui::PopStyleColor(3);
    ImGui::End();
}

void EasyPlayground::ImGUI_ChampionSelectWindow()
{
	static bool loaded = false;
	static std::vector<ImTextureID> icons;

	if (!loaded)
	{
		icons.push_back(LoadTextureSTB(GetPath("res/images/champions/1.png").c_str()));
		icons.push_back(LoadTextureSTB(GetPath("res/images/champions/2.png").c_str()));
		icons.push_back(LoadTextureSTB(GetPath("res/images/champions/3.png").c_str()));
		icons.push_back(LoadTextureSTB(GetPath("res/images/champions/4.png").c_str()));
		icons.push_back(LoadTextureSTB(GetPath("res/images/champions/5.png").c_str()));
		loaded = true;
	}

	int clicked = DrawChampionSelectWindow(
		icons
	);

	if (clicked >= 0)
	{
		// Do something
		printf("Icon %d clicked!\n", clicked);
		championSelected = clicked + 1;
		champSelect = false;
	}
}

void EasyPlayground::ImGUI_LoginStatusWindow()
{
	if (!loginInProgress || champSelect)
		return;

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
	float textWidth = ImGui::CalcTextSize(loginStatusText.c_str()).x;

	CenterItem(textWidth);
	ImGui::Text("%s", loginStatusText.c_str());

	ImGui::Spacing();
	ImGui::Spacing();

	// Center button
	float btnWidth = 100.0f;
	CenterItem(btnWidth);

	if (!loginFailed)
	{
		if (ImGui::Button("Cancel", ImVec2(btnWidth, 28)))
		{
            Logout();
		}
	}
	else
	{
		if (ImGui::Button("OK", ImVec2(btnWidth, 28)))
		{
			Logout();
		}
	}

	ImGui::End();
}

void EasyPlayground::ImGUI_BroadcastMessageWindow()
{
    if (!isBroadcastMessage || broadcastMessage.length() == 0)
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
    float textWidth = ImGui::CalcTextSize(broadcastMessage.c_str()).x;

    CenterItem(textWidth);
    ImGui::Text("%s", broadcastMessage.c_str());

    ImGui::Spacing();
    ImGui::Spacing();

    // Center button
    float btnWidth = 100.0f;
    CenterItem(btnWidth);
    if (ImGui::Button("OK", ImVec2(btnWidth, 28)))
    {
        isBroadcastMessage = false;
        broadcastMessage = "";
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
	CenteredText("Username");
	CenteredItem(240);
	ImGui::InputText("##username", usernameArr, IM_ARRAYSIZE(usernameArr));
	ImGui::Spacing();

	// Password
	CenteredText("Password");
	CenteredItem(240);
	ImGui::InputText("##password", passwordArr, IM_ARRAYSIZE(passwordArr), ImGuiInputTextFlags_Password);
	ImGui::Spacing();

	// Remember me
	{
		float w = ImGui::CalcTextSize("Remember me").x + 30.0f;
		CenteredItem(w);
		ImGui::Checkbox("Remember me", &rememberMe);
	}

	ImGui::Spacing();
	ImGui::Spacing();

	// Login button
	float btnWidth = 140;
	CenteredItem(btnWidth);
	if (ImGui::Button("Login", ImVec2(btnWidth, 32)))
	{
		loginFailed = false;
		loginInProgress = true;
		loginStatusWindow = true;

		// Save config on click
		std::string u = usernameArr;
		std::string p = passwordArr;

		if (!loginThreadRunning)
		{
			loginThreadRunning = true;
			loginThread = std::thread([u, p]() {Login(u, p); });
			loginThread.detach();




		}
	}

	ImGui::End();
}

void EasyPlayground::ImGUIRender()
{
	if (loggedIn)
		isRender = true;

	static bool show_demo_window = true;
	static bool show_another_window = false;
	static ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	// Start the Dear ImGui frame
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	ImGui::SetNextWindowPos({ 0,0 });

	if (champSelect)
		ImGUI_ChampionSelectWindow();

    ImGUI_PlayerInfoWindow();

	ImGui::SetNextWindowPos({ 0,0 });

	if (!loggedIn)
	{
		if (loginStatusWindow)
			ImGUI_LoginStatusWindow();
		else
			ImGUI_LoginWindow();
	}
	else
	{
		ImGui::SetNextWindowPos({ 0,0 });
		ImGui::Begin("ImGUI Settings");
		ImGui::Checkbox("Fog Enabled", &imgui_isFog);
		ImGui::Checkbox("Triangles Enabled", &imgui_triangles);
		ImGui::Checkbox("Normals Enabled", &imgui_showNormalLines);
		ImGui::InputFloat("Normal Length", &imgui_showNormalLength);
		ImGui::End();

        ImGUI_BroadcastMessageWindow();

        ImGUI_DrawChatWindow();
	}

	// Rendering
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
