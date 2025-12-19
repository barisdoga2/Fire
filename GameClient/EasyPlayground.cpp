#include "EasyPlayground.hpp"

#include <iostream>
#include <chrono>
#include <sstream>
#include <iomanip>

#include <imgui.h>
#include <imgui_internal.h>
#include <../3rd_party/imgui_impl_opengl3.h>
#include <../3rd_party/imgui_impl_glfw.h>

#include <EasyDisplay.hpp>
#include <EasyCamera.hpp>
#include <EasyShader.hpp>
#include <EasyModel.hpp>
#include <EasyAnimation.hpp>
#include <EasyAnimator.hpp>
#include <EasyUtils.hpp>
#include <EasyCamera.hpp>
#include <EasyBuffer.hpp>
#include <EasyTexture.hpp>
#include <HDR.hpp>
#include <EasyRender.hpp>
#include <SkyboxRenderer.hpp>
#include <ChunkRenderer.hpp>
#include <ModelRenderer.hpp>
#include <DebugRenderer.hpp>
#include <GL_Ext.hpp>
#include <EasyIO.hpp>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "EasyPlayground_ImGUI.hpp"
#include "EasyPlayground_Network.hpp"
#include "EasyPlayground_Loading.hpp"

#include <EasyConsole.hpp>


EasyPlayground::EasyPlayground()
{
	
}

EasyPlayground::~EasyPlayground()
{
	DeInit();
}

// Render
bool EasyPlayground::Render(double _dt)
{
	camera->Update(_dt);

	for (Player* e : players)
		if (e != player)
			e->Update(_dt);
		else
			player->Update((TPCamera*)camera, _dt);

	bool success = true;

	// Render
	if (isRender || isTestRender)
		RenderScene(_dt);

	// ImGUI
	if (!isTestRender)
		EasyPlayground::ImGUI_Render(_dt);
	else
		EasyPlayground::ImGUI_TestRender();

	return success;
}

void EasyPlayground::RenderScene(double _dt)
{
	std::vector<EasyEntity*> batch{};
	for (auto& [name, entity] : entities)
		batch.push_back(entity);

	// Skybox
	SkyboxRenderer::Render(camera);

	// Normal Lines
	DebugRenderer::Render(camera, batch, chunks, renderData);

	if (renderData.imgui_triangles) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	// Map
	ChunkRenderer::Render(camera, chunks, hdr, renderData.imgui_isFog);

	// Objects
	ModelRenderer::Render(camera, batch, renderData);

	// Players
	std::vector<EasyEntity*> playersAsEntity{};
	for (Player* p : players)
		playersAsEntity.push_back(p);
	ModelRenderer::Render(camera, playersAsEntity, renderData);

	if (renderData.imgui_triangles) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void EasyPlayground::ImGUI_Render(double _dt)
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	ImGUI_DrawConsoleWindow();

	if (network->IsInGame())
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

void EasyPlayground::StartRender(double _dt)
{
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

void EasyPlayground::EndRender(double _dt)
{
	EasyDisplay::SetTitle((std::ostringstream() << std::fixed << std::setprecision(3) << "FPS: " << EasyDisplay::RtFPS() << " | UPS: " << EasyDisplay::RtUPS()).str().c_str());
	EasyDisplay::Render(_dt);
}

// Update
bool EasyPlayground::Update(double _dt)
{
	EasyDisplay::Update(_dt);

	// Try load everything until done
	static bool srcReady{}; if (!srcReady && Model(MAIN_CHARACTER)->LoadToGPU()) srcReady = true;

	isRender = network->IsInGame();

	NetworkUpdate(_dt);

	return !EasyDisplay::ShouldClose();
}

void EasyPlayground::NetworkUpdate(double _dt)
{
	network->Update();

	if (!network->IsInGame())
		return;

	// Heartbeat
	{
		static Timestamp_t nextHeartbeat = Clock::now();
		Millis_t heartbeatPeriod(1000U);
		if (Clock::now() >= nextHeartbeat)
		{
			nextHeartbeat = Clock::now() + heartbeatPeriod;

			std::cout << "[EasyPlayground] NetworkUpdate - Heartbeat sent.\n";
			network->GetSendCache().push_back(new sHearbeat());
		}
	}

	// Process Received
	ProcessReceived(_dt);
}

// Input
bool EasyPlayground::button_callback(const MouseData& data) 
{ 
	if (ImGui_ImplGlfw_MouseButtonCallback(data.window, data.button.button, data.button.action, data.button.mods); ImGui::GetIO().WantCaptureMouse)
		return true;

	if (player && player->button_callback(data))
		return true;

	if (camera->enabled && camera->button_callback(data))
		return true;

	return false;
}

bool EasyPlayground::scroll_callback(const MouseData& data) 
{ 
	if (ImGui_ImplGlfw_ScrollCallback(data.window, data.scroll.now.x, data.scroll.now.y); ImGui::GetIO().WantCaptureMouse)
		return true;

	if (player && player->scroll_callback(data))
		return true;

	if (camera->enabled && camera->scroll_callback(data))
		return true;

	return false;
}

bool EasyPlayground::move_callback(const MouseData& data)
{
	if (ImGui_ImplGlfw_CursorPosCallback(data.window, data.position.now.x, data.position.now.y); ImGui::GetIO().WantCaptureMouse)
		return true;

	if (player && player->move_callback(data))
		return true;

	if (camera->enabled && camera->move_callback(data))
		return true;

	return false;
}

bool EasyPlayground::ShortcutManagement(const KeyboardData& data)
{
	static const int exitKey = GLFW_KEY_ESCAPE;
	static const int consoleKey = GLFW_KEY_GRAVE_ACCENT;
	static const int testRenderKey = GLFW_KEY_F1;
	static int lastKey = 0;

	bool captured = false;

	if (data.key == exitKey || data.key == consoleKey || data.key == testRenderKey)
	{
		if (data.action == GLFW_RELEASE)
		{
			if (data.key == exitKey)
			{
				captured = true;
			}
			else if (data.key == consoleKey)
			{
				isConsoleWindow = !isConsoleWindow;
				captured = true;
			}
			else if (data.key == testRenderKey)
			{
				isTestRender = !isTestRender;
				if (isTestRender)
				{
					ClearPlayers();
					CreateMainPlayer(network, 0U);
				}
				ImGui::FocusWindow(nullptr);
				captured = true;
			}
		}
		else if (data.action == GLFW_PRESS || data.action == GLFW_REPEAT)
		{
			captured = true;
		}
	}

	lastKey = data.key;

	return captured;
}

bool EasyPlayground::key_callback(const KeyboardData& data) 
{
	if (ShortcutManagement(data))
		return true;

	if (ImGui_ImplGlfw_KeyCallback(data.window, data.key, data.scancode, data.action, data.mods); ImGui::GetIO().WantCaptureKeyboard)
		return true;

	if (player && player->key_callback(data))
		return true;

	if (camera->enabled && camera->key_callback(data))
		return true;

	return false;
}

bool EasyPlayground::char_callback(const KeyboardData& data) 
{ 
	if (ImGui_ImplGlfw_CharCallback(data.window, data.codepoint); ImGui::GetIO().WantCaptureKeyboard)
		return true;

	if (player && player->char_callback(data))
		return true;

	if (camera->enabled && camera->char_callback(data))
		return true;

	return false;
}

// Utility
Player* EasyPlayground::GetPlayerByUID(UserID_t uid)
{
	Player* ret{};
	for (Player* p : players)
	{
		if (p->UserID() == uid)
		{
			ret = p;
			break;
		}
	}
	return ret;
}
