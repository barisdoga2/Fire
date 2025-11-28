#include "ClientTest.hpp"
#include "../Game Server/Net.hpp"


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

inline void Login(std::string username_, std::string password)
{
	loggedIn = false;
	loginFailed = false;
	if (crypt)
	{
		crypt->key = {};
		crypt->sequence_id_in = 0U;
		crypt->sequence_id_out = 0U;
		crypt->session_id = 0U;
	}

	// Set login status text
	loginStatusText = "Logging in...";

	// Get key, user id and session id from web api
	std::string response = client.ClientWebRequest(SERVER_URL, username_, password);
	std::string prefix = "<textarea name=\"jwt\" readonly>";
	std::string prefix2 = "<div class=\"msg\">";
	if (response._Starts_with("Curl error:"))
	{
		loginStatusText = response.substr(std::string("Curl error:").length());
		loginFailed = true;
	}
	else
	{
		if (size_t index = response.find(prefix); index != std::string::npos)
		{
			loginStatusText = "Waiting the game server...";

			// Parse data and 'remember me'
			response = response.substr(index + prefix.length());
			response = response.substr(0, response.find("</textarea>"));
			sessionID = static_cast<SessionID_t>(std::stoul(response.substr(0, response.find_first_of(":"))));
			response = response.substr(response.find_first_of(":") + 1U);
			userID = static_cast<uint32_t>(std::stoul(response.substr(0, response.find_first_of(":"))));
			response = response.substr(response.find_first_of(":") + 1U);
			std::string key = response;

			Key_t key_t(KEY_SIZE);
			memcpy(key_t.data(), key.data(), KEY_SIZE);
			crypt = new PeerCryptInfo(sessionID, 0U, 0U, key_t);
			username = username_;

			if (rememberMe)
				SaveConfig(true, usernameArr, passwordArr);
			else
				SaveConfig(false, "", "");
		}
		else
		{
			if (size_t index = response.find(prefix2); index != std::string::npos)
			{
				response = response.substr(index + prefix2.length());
				response = response.substr(0, response.find("</div>"));
				loginStatusText = response;
				loginFailed = true;
			}
			else
			{
				loginStatusText = "Error while parsing response!";
				loginFailed = true;
			}
		}
	}

	// Start communicating with game server
	if (!loginFailed)
	{
		// Start inline receive and send threads
		std::vector<EasySerializeable*> sendObjs = {  }, recvObjs = {  };
		std::mutex sendObjs_m, recvObjs_m;
		std::thread recvTh = std::thread([&recvObjs, &recvObjs_m]
			{
				while (!loginFailed)
				{
					if (crypt && recvObjs_m.try_lock())
					{
						uint64_t recv = client.ClientReceive(*crypt, recvObjs);
						if (recv == 1U)
						{

						}
						recvObjs_m.unlock();
					}
					std::this_thread::sleep_for(std::chrono::milliseconds(100U));
				}
			}
		);
		std::thread sendTh = std::thread([&sendObjs, &sendObjs_m]
			{
				while (!loginFailed)
				{
					if (crypt && sendObjs_m.try_lock())
					{
						uint64_t send = client.ClientSend(*crypt, sendObjs);
						if (send == 1U)
						{

						}
						sendObjs_m.unlock();
					}
					std::this_thread::sleep_for(std::chrono::milliseconds(100U));
				}
			}
		);

		// Send initial packet(Login request)
		if (!loginFailed)
		{
			sendObjs_m.lock();
			sendObjs.push_back(new pHello("Heeey!"));
			sendObjs_m.unlock();
		}

		// Wait Login Response
		if(!loginFailed)
		{
			bool loginResponseReceived{};
			unsigned int timeoutCounter = 30U;
			while (!loginResponseReceived && timeoutCounter != 0U)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(100U));

				recvObjs_m.lock();
				for (std::vector<EasySerializeable*>::iterator it = recvObjs.begin(); it != recvObjs.end() && !loginResponseReceived; )
				{
					if (sLoginResponse* loginResponse = dynamic_cast<sLoginResponse*>(*it); loginResponse)
					{
						loginResponseReceived = true;

						// Set Message
						loginStatusText = loginResponse->message;
						loginFailed = !loginResponse->response;

						// Delete Packet
						delete loginResponse;
						it = recvObjs.erase(it);
					}
					else
					{
						it++;
					}
				}
				recvObjs_m.unlock();
				timeoutCounter--;
			}
			if (timeoutCounter == 0U && !loginResponseReceived && !loginFailed)
			{
				loginStatusText = "Login response timeout!";
				loginFailed = true;
			}
		}

		// Wait Player Boot Info
		if (!loginFailed)
		{
			bool playerBootInfoReceived{};
			unsigned int timeoutCounter = 30U;
			while (!playerBootInfoReceived && timeoutCounter != 0U)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(100U));

				recvObjs_m.lock();
				for (std::vector<EasySerializeable*>::iterator it = recvObjs.begin(); it != recvObjs.end() && !playerBootInfoReceived ; )
				{
					if (sPlayerBootInfo* playerBootInfo = dynamic_cast<sPlayerBootInfo*>(*it); playerBootInfo)
					{
						loginStatusText = "Player boot received!";

						playerBootInfoReceived = true;

						// Set Message
						stats.userID = playerBootInfo->userID;
						stats.gametime = playerBootInfo->gametime;
						stats.golds = playerBootInfo->golds;
						stats.diamonds = playerBootInfo->diamonds;
						stats.tutorial_done = playerBootInfo->tutorialDone;
						stats.champions_owned = playerBootInfo->championsOwned;

						// Delete Packet
						delete playerBootInfo;
						it = recvObjs.erase(it);
					}
					else
					{
						it++;
					}
				}
				recvObjs_m.unlock();
				timeoutCounter--;
			}
			if (timeoutCounter == 0U && !playerBootInfoReceived && !loginFailed)
			{
				loginStatusText = "Player boot timeout!";
				loginFailed = true;
			}
		}

		// Send champion select request
		if (!loginFailed)
		{
			championSelected = 0U;
			champSelect = true;
			while (champSelect)
			{

			}
			loginStatusText = "Selecting champion!";
			sendObjs_m.lock();
			sendObjs.push_back(new sChampionSelectRequest(championSelected));
			sendObjs_m.unlock();
		}

		// Wait champion select response
		if (!loginFailed)
		{
			bool championSelectResponseReceived{};
			unsigned int  timeoutCounter = 30U;
			while (!championSelectResponseReceived && timeoutCounter != 0U)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(100U));

				recvObjs_m.lock();
				for (std::vector<EasySerializeable*>::iterator it = recvObjs.begin(); it != recvObjs.end() && !championSelectResponseReceived ; )
				{
					if (sChampionSelectResponse* championSelectResponse = dynamic_cast<sChampionSelectResponse*>(*it); championSelectResponse)
					{
						championSelectResponseReceived = true;

						// Set Message
						loginFailed = !championSelectResponse->response;
						loginStatusText = championSelectResponse->message;

						// Delete Packet
						delete championSelectResponse;
						it = recvObjs.erase(it);
					}
					else
					{
						it++;
					}
				}
				recvObjs_m.unlock();
				timeoutCounter--;
			}
			if (timeoutCounter == 0U && !championSelectResponseReceived && !loginFailed)
			{
				loginStatusText = "Champion select response timeout!";
				loginFailed = true;
			}
		}

		// If champ relect response receive then we logged in
		loggedIn = !loginFailed;

		// Recv/Send while logged in 
		while (loggedIn)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100U));

			recvObjs_m.lock();
			for (std::vector<EasySerializeable*>::iterator it = recvObjs.begin(); it != recvObjs.end() ; )
			{
				if (sDisconnectResponse* disconnectResponse = dynamic_cast<sDisconnectResponse*>(*it); disconnectResponse)
				{
					loggedIn = false;
					loginFailed = true;

					// Set Message
					loginStatusText = disconnectResponse->message;

					// Delete Packet
					delete disconnectResponse;
					it = recvObjs.erase(it);
				}
				else
				{
					it++;
				}
			}
			recvObjs_m.unlock();

			static unsigned int ctr = 10U;
			--ctr;
			if (ctr == 0U)
			{
				sendObjs_m.lock();
				sendObjs.push_back(new sHearbeat());
				sendObjs_m.unlock();
				ctr = 10U;
			}
		}

		if(sendTh.joinable())
			sendTh.join();
		if (recvTh.joinable())
			recvTh.join();
	}

	loginThreadRunning = false;
	isRender = false;
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

int DrawChampionSelectWindow(
	const std::vector<ImTextureID>& icons,   // images
	float buttonSize = 64.0f,
	float padding = 10.0f)
{
	int N = (int)icons.size();
	if (N == 0)
		return -1;

	// ---- Auto grid size ----
	int cols = (int)std::floor(std::sqrt((float)N));
	if (cols < 1) cols = 1;
	int rows = (N + cols - 1) / cols;

	// Window size auto-fit
	ImVec2 winSize(
		350,
		400   // header labels height
	);
	ImGui::SetNextWindowSize(winSize, ImGuiCond_Always);

	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));

	ImGui::Begin("Player Info");

	// ---- Header Labels ----
	ImGui::Text("Username:   %s", username.c_str());
	ImGui::Text("User ID:    %u", userID);
	ImGui::Text("Session ID: %u", sessionID);
	ImGui::Separator();

	ImGui::Text("Gold:    %u", stats.golds);
	ImGui::Text("Diamond: %u", stats.diamonds);
	ImGui::Text("Game Time: %.1f hours", stats.gametime);
	ImGui::Text("Champions Owned: %d", stats.champions_owned.size());

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	// ---- GRID ----
	int clicked = -1;

	for (int i = 0; i < N; i++)
	{
		bool owned = std::find(stats.champions_owned.begin(), stats.champions_owned.end(), i + 1) != stats.champions_owned.end();

		ImGui::PushID(i);
		if (!owned)
		{
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.40f);
			ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
		}
		bool pressed = ImGui::ImageButton(std::to_string(i).c_str(), icons[i], ImVec2(buttonSize, buttonSize));
		if (pressed && owned)
			clicked = i;
		if (!owned)
		{
			ImGui::PopItemFlag();
			ImGui::PopStyleVar();
		}
		ImGui::PopID();

		if ((i + 1) % cols != 0)
			ImGui::SameLine(padding);
	}

	ImGui::End();
	return clicked;
}

void EasyPlayground::ImGUI_ChampionSelectWindow()
{
	static bool loaded = false;
	static std::vector<ImTextureID> icons;

	if (!loaded)
	{
		icons.push_back(LoadTextureSTB(GetPath("res/images/champions/1.png").c_str()));
		icons.push_back(LoadTextureSTB(GetPath("res/images/champions/2.png").c_str()));
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

	ImVec2 winSize(350, 120);   // smaller window
	ImVec2 center = ImGui::GetMainViewport()->GetCenter();

	// Center on both X and Y precisely
	ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowSize(winSize, ImGuiCond_Always);

	ImGui::Begin("LoginStatus", nullptr,
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
			loginStatusWindow = false;
			loginInProgress = false;
			loginFailed = true;

			if (crypt)
			{
				delete crypt;
				crypt = nullptr;
			}

			username = "";

		}
	}
	else
	{
		if (ImGui::Button("OK", ImVec2(btnWidth, 28)))
		{
			loginStatusWindow = false;
			loginInProgress = false;
			loginFailed = true;
		}
	}

	ImGui::End();
}

void EasyPlayground::ImGUI_LoginWindow()
{
	ImVec2 winSize = ImVec2(350, 260);
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
	}

	// Rendering
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
