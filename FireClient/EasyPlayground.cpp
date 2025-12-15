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
#include <EasyRenderer.hpp>
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



EasyPlayground::EasyPlayground(EasyDisplay* display, EasyBufferManager* bm) : display(display), bm(bm), network(new ClientNetwork(bm, this))
{
	
}

EasyPlayground::~EasyPlayground()
{
	network->Stop();

	// ImGUI
	{
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}

	delete shader;
	delete normalLinesShader;
	delete camera;
	delete hdr;
	delete network;
	delete model;
	delete cube_1x1x1;
	delete buildings;
	delete walls;
	for (auto& c : chunks)
		delete c;

	HDR::DeInit();
	SkyboxRenderer::DeInit();
	ChunkRenderer::DeInit();
}

bool EasyPlayground::Init()
{
	// Inputs
	{
		EasyKeyboard::Init(*display);
		EasyMouse::Init(*display);

		EasyKeyboard::AddListener(this);
		EasyMouse::AddListener(this);
	}

	// Assets
	{
		ReloadAssets();
	}

	// Shaders
	{
		ReloadShaders();
	}

	// ImGUI
	{
		ImGUI_Init();
	}

	return true;
}

bool EasyPlayground::Render(double _dt)
{
	camera->Update(_dt);

	bool success = true;
	static bool srcReady{};
	if (!srcReady && model->LoadToGPU())
	{
		for (const auto& kv : model->instances)
			for (auto& kv2 : kv.second)
				kv2->scale = glm::vec3(0.0082f); // Y = 1.70m
		srcReady = true;
	}

	// Render
	if (isRender || isTestRender)
	{
		std::vector<EasyEntity*> entities = {  };

		// Skybox
		SkyboxRenderer::Render(camera);

		// Normal Lines
		DebugRenderer::Render(camera, entities, chunks, renderData);
		
		if (renderData.imgui_triangles) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

		// Map
		ChunkRenderer::Render(camera, chunks, hdr, renderData.imgui_isFog);

		// Objects
		ModelRenderer::Render(camera, entities, renderData);

		// Players
		std::vector<EasyEntity*> playersAsEntity{};
		for (Player* p : players)
			playersAsEntity.push_back(p);
		ModelRenderer::Render(camera, playersAsEntity, renderData);

		if (renderData.imgui_triangles) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	// ImGUI
	if (!isTestRender)
		EasyPlayground::ImGUI_Render();
	else
		EasyPlayground::ImGUI_TestRender();

	return success;
}

bool EasyPlayground::Update(double _dt)
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

	if(player)
		player->Update(_dt);
	for(EasyEntity* e : players)
		e->Update(_dt);

	isRender = network->IsInGame();

	NetworkUpdate(_dt);

	return !display->ShouldClose();
}

void EasyPlayground::StartRender(double _dt)
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

void EasyPlayground::EndRender()
{
	glfwSetWindowTitle(display->window, (std::ostringstream() << std::fixed << std::setprecision(3) << "FPS: " << fps << " | UPS: " << ups).str().c_str());
	glfwSwapBuffers(display->window);
	glfwPollEvents();
}

bool EasyPlayground::mouse_callback(const MouseData& md) 
{ 
	if (!camera->mode)
	{
		ImGui_ImplGlfw_MouseButtonCallback(md.window, md.button.button, md.button.action, md.button.mods);
		if (ImGui::GetIO().WantCaptureMouse)
			return false;
	}

	camera->mouse_callback(md);

	return false; 
}

bool EasyPlayground::scroll_callback(const MouseData& md) 
{ 
	if (!camera->mode)
	{
		ImGui_ImplGlfw_ScrollCallback(md.window, md.scroll.now.x, md.scroll.now.y);
		if (ImGui::GetIO().WantCaptureMouse)
			return false;
	}

	camera->scroll_callback(md);

	return false; 
}

bool EasyPlayground::cursorMove_callback(const MouseData& md) 
{ 
	if (!camera->mode)
	{
		ImGui_ImplGlfw_CursorPosCallback(md.window, md.position.now.x, md.position.now.y);
		if (ImGui::GetIO().WantCaptureMouse)
			return false;
	}

	camera->cursorMove_callback(md);

	return false;
}

bool EasyPlayground::key_callback(const KeyboardData& data) 
{ 
	if (data.key == GLFW_KEY_GRAVE_ACCENT && data.action == GLFW_RELEASE)
	{
		isConsoleWindow = !isConsoleWindow;
		return false;
	}

	if (data.key == GLFW_KEY_F1 && data.action == GLFW_RELEASE)
	{
		isTestRender = !isTestRender;
		if (isTestRender)
		{
			for (auto& p : players)
				delete p;
			players.clear();
			player = new Player(model, 0U, true, glm::vec3(0, 0, 0));
			players.push_back(player);
		}
		return false;
	}

	if (data.key == GLFW_KEY_LEFT_ALT && data.action == GLFW_RELEASE)
	{
		camera->ModeSwap(false);
		return false;
	}
	else if (data.key == GLFW_KEY_LEFT_ALT && data.action == GLFW_PRESS)
	{
		camera->ModeSwap(true);
		return false;
	}

	ImGui_ImplGlfw_KeyCallback(data.window, data.key, data.scancode, data.action, data.mods);
	if (ImGui::GetIO().WantCaptureKeyboard)
		return false;

	if (camera->key_callback(data))
	{
		return false;
	}

	if (data.key == GLFW_KEY_ESCAPE && data.action == GLFW_RELEASE)
	{
		display->exitRequested = true;
		return false;
	}

	if (data.key == GLFW_KEY_T && data.action == GLFW_RELEASE)
	{
		animation = 2;
		return false;
	}

	if (data.key == GLFW_KEY_W && data.action == GLFW_RELEASE)
	{
		network->GetSendCache().push_back(new sMoveInput(network->session.uid, 0U, 0U, glm::vec3(0.0f), 0U));
		return false;
	}

	if (data.key == GLFW_KEY_R && data.action == GLFW_RELEASE)
	{
		ReloadShaders();
		return false;
	}

	if (data.key == GLFW_KEY_P && data.action == GLFW_RELEASE)
	{
		ReGenerateMap();
		return false;
	}

	return false; 
}

bool EasyPlayground::character_callback(const KeyboardData& data) 
{ 
	if (!camera->mode)
	{
		ImGui_ImplGlfw_CharCallback(data.window, data.codepoint);
		if (ImGui::GetIO().WantCaptureKeyboard)
			return false;
	}

	return false;
}

void EasyPlayground::ForwardStandartIO()
{
	using namespace UIConsole;
	gImGuiCoutBuf = new ImGuiConsoleBuf(gConsoleLines);
	gOldCoutBuf = std::cout.rdbuf(gImGuiCoutBuf);
}
