#include "ClientTest.hpp"

#ifdef REMOTE
#define SERVER_IP "31.210.43.142"
#else
#define SERVER_IP "127.0.0.1"
#endif

#define SERVER_PORT 54000U
#define SERVER_URL "https://barisdoga.com/index.php"

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
std::string loginStatusText;
std::thread loginThread;
std::atomic<bool> loginThreadRunning = false;

inline void Login(std::string username_, std::string password)
{
	loginStatusText = "Logging in...";

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

			loginStatusText = "Waiting the game server...";
			std::cout << "Login OK! SessionID: " << sessionID << ", UserID: " << userID << ", Key: " << key << std::endl;
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
}

void EasyPlayground::ImGUI_LoginStatusWindow()
{
	if (!loginInProgress)
		return;

	ImVec2 winSize(250, 80);   // smaller window
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

	auto CenterItem = [&](float width)
		{
			ImGui::SetCursorPosX((winSize.x - width) * 0.5f);
		};

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
			loginFailed = false;

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
			loginFailed = false;
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

	CenteredText("Welcome");
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

		loginThreadRunning = true;
		loginThread = std::thread([u, p](){Login(u, p); loginThreadRunning = false; });
		loginThread.detach();
	}

	ImGui::End();
}

void EasyPlayground::ImGUIRender()
{
	static bool show_demo_window = true;
	static bool show_another_window = false;
	static ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	// Start the Dear ImGui frame
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	ImGui::SetNextWindowPos({ 0,0 });

	ImGui::Begin("ImGUI Settings");
	ImGui::Checkbox("ImGUI Enabled", &imgui_en);
	ImGui::Checkbox("Fog Enabled", &imgui_isFog);
	ImGui::Checkbox("Triangles Enabled", &imgui_triangles);
	ImGui::Checkbox("Normals Enabled", &imgui_showNormalLines);
	ImGui::InputFloat("Normal Length", &imgui_showNormalLength);
	ImGui::End();


	if (loginStatusWindow)
		ImGUI_LoginStatusWindow();
	else
		ImGUI_LoginWindow();

	if (imgui_en)
	{
		// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
		if (show_demo_window)
			ImGui::ShowDemoWindow(&show_demo_window);

		// 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
		{
			static float f = 0.0f;
			static int counter = 0;

			ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

			ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
			ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
			ImGui::Checkbox("Another Window", &show_another_window);

			ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
			ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

			if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
				counter++;
			ImGui::SameLine();
			ImGui::Text("counter = %d", counter);

			//ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
			ImGui::End();
		}

		// 3. Show another simple window.
		if (show_another_window)
		{
			ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
			ImGui::Text("hello");
			if (ImGui::Button("Close Me"))
				show_another_window = false;
			ImGui::End();
		}
	}
	else
	{
		//ImGui::SetWindowFocus(nullptr);
	}

	// Rendering
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
