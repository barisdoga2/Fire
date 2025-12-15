#include "EasyPlayground.hpp"

#include <iostream>
#include <chrono>
#include <sstream>
#include <iomanip>


#include <imgui.h>
#include <imgui_internal.h>
#include "../3rd_party/imgui_impl_opengl3.h"
#include "../3rd_party/imgui_impl_glfw.h"

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
#include <SkyboxRenderer.hpp>
#include <ChunkRenderer.hpp>
#include <GL_Ext.hpp>

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
	// Callbacks
	{
		static EasyPlayground* instance = this;
		glfwSetKeyCallback(display->window, [](GLFWwindow* w, int k, int s, int a, int m) { instance->key_callback(w, k, s, a, m); });
		glfwSetCursorPosCallback(display->window, [](GLFWwindow* w, double x, double y) { instance->cursor_callback(w, x, y); });
		glfwSetMouseButtonCallback(display->window, [](GLFWwindow* window, int button, int action, int mods) { instance->mouse_callback(window, button, action, mods); });
		glfwSetScrollCallback(display->window, [](GLFWwindow* w, double x, double y) { instance->scroll_callback(w, x, y); });
		glfwSetCharCallback(display->window, [](GLFWwindow* w, unsigned int x) { instance->char_callback(w, x); });
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
		std::vector<EasyModel*> objs = { walls };


		// Skybox
		SkyboxRenderer::Render(camera);

		// Normal Lines
		if (imgui_showNormalLines)
		{
			normalLinesShader->Start();

			normalLinesShader->LoadUniform("view", camera->view_);
			normalLinesShader->LoadUniform("proj", camera->projection_);
			normalLinesShader->LoadUniform("normalLength", imgui_showNormalLength);

			// Objects
			for (EasyModel* model : objs)
			{
				for (const auto& kv : model->instances)
				{
					EasyModel::EasyMesh* mesh = kv.first;

					if (!mesh->LoadToGPU())
						continue;

					GL(BindVertexArray(mesh->vao));
					GL(EnableVertexAttribArray(0));
					GL(EnableVertexAttribArray(2));

					for (EasyTransform* t : kv.second)
					{
						normalLinesShader->LoadUniform("model",
							CreateTransformMatrix(t->position, t->rotationQuat, t->scale));

						glDrawElements(GL_TRIANGLES,
							static_cast<unsigned int>(mesh->indices.size()),
							GL_UNSIGNED_INT, 0);
					}

					GL(DisableVertexAttribArray(2));
					GL(DisableVertexAttribArray(0));
					GL(BindVertexArray(0));
				}
			}

			// Chunks
			for (Chunk* chunk : chunks)
			{
				glBindVertexArray(chunk->VAO);
				glEnableVertexAttribArray(0);
				normalLinesShader->LoadUniform("model", glm::translate(glm::mat4x4(1), glm::vec3(chunk->coord.x * Chunk::CHUNK_SIZE, 0, chunk->coord.y * Chunk::CHUNK_SIZE)));

				glDrawElements(GL_TRIANGLES, (GLint)chunk->indices.size(), GL_UNSIGNED_INT, 0);

				glDisableVertexAttribArray(0);
				glBindVertexArray(0);
			}

			normalLinesShader->Stop();

		}

		if (imgui_triangles) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

		// Map
		ChunkRenderer::Render(camera, chunks, hdr, imgui_isFog);

		// Objects
		{
			shader->Start();
			shader->LoadUniform("view", camera->view_);
			shader->LoadUniform("proj", camera->projection_);
			shader->LoadUniform("uCameraPos", camera->position);
			shader->LoadUniform("uIsFog", imgui_isFog ? 1.0f : 0.0f);

			for (EasyModel* model : objs)
			{
				//if (model->animator)
				//	shader->LoadUniform("boneMatrices", model->animator->GetFinalBoneMatrices());

				for (const auto& kv : model->instances)
				{
					EasyModel::EasyMesh* mesh = kv.first;

					if (!mesh->LoadToGPU())
						continue;

					GL(BindVertexArray(mesh->vao));
					GL(EnableVertexAttribArray(0));
					GL(EnableVertexAttribArray(1));
					GL(EnableVertexAttribArray(2));
					GL(EnableVertexAttribArray(3));
					GL(EnableVertexAttribArray(4));
					GL(EnableVertexAttribArray(5));
					GL(EnableVertexAttribArray(6));

					glActiveTexture(GL_TEXTURE0);
					glBindTexture(GL_TEXTURE_2D, mesh->texture);
					shader->LoadUniform("diffuse", 0);

					//shader->LoadUniform("animated", mesh->animatable ? 1 : 0);
					shader->LoadUniform("animated", 0);

					for (EasyTransform* t : kv.second)
					{
						shader->LoadUniform("model", CreateTransformMatrix(t->position, t->rotationQuat, t->scale));
						glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(mesh->indices.size()), GL_UNSIGNED_INT, 0);
					}

					GL(DisableVertexAttribArray(6));
					GL(DisableVertexAttribArray(5));
					GL(DisableVertexAttribArray(4));
					GL(DisableVertexAttribArray(3));
					GL(DisableVertexAttribArray(2));
					GL(DisableVertexAttribArray(1));
					GL(DisableVertexAttribArray(0));
					GL(BindVertexArray(0));
				}
			}

			shader->Stop();
		}

		// Players
		{
			shader->Start();
			shader->LoadUniform("view", camera->view_);
			shader->LoadUniform("proj", camera->projection_);
			shader->LoadUniform("uCameraPos", camera->position);
			shader->LoadUniform("uIsFog", imgui_isFog ? 1.0f : 0.0f);

			for (EasyEntity* entity : players)
			{
				if (entity->animator)
					shader->LoadUniform("boneMatrices", entity->animator->GetFinalBoneMatrices());

				for (const auto& kv : entity->model->instances)
				{
					EasyModel::EasyMesh* mesh = kv.first;

					if (!mesh->LoadToGPU())
						continue;

					GL(BindVertexArray(mesh->vao));
					GL(EnableVertexAttribArray(0));
					GL(EnableVertexAttribArray(1));
					GL(EnableVertexAttribArray(2));
					GL(EnableVertexAttribArray(3));
					GL(EnableVertexAttribArray(4));
					GL(EnableVertexAttribArray(5));
					GL(EnableVertexAttribArray(6));

					glActiveTexture(GL_TEXTURE0);
					glBindTexture(GL_TEXTURE_2D, mesh->texture);
					shader->LoadUniform("diffuse", 0);

					shader->LoadUniform("animated", entity->animator ? 1 : 0);

					for (EasyTransform* t : kv.second)
					{
						shader->LoadUniform("model", CreateTransformMatrix(t->position, t->rotationQuat, t->scale));
						glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(mesh->indices.size()), GL_UNSIGNED_INT, 0);
					}

					GL(DisableVertexAttribArray(6));
					GL(DisableVertexAttribArray(5));
					GL(DisableVertexAttribArray(4));
					GL(DisableVertexAttribArray(3));
					GL(DisableVertexAttribArray(2));
					GL(DisableVertexAttribArray(1));
					GL(DisableVertexAttribArray(0));
					GL(BindVertexArray(0));
				}
			}

			shader->Stop();
		}

		if (imgui_triangles) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
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
		player->Update(_dt, mb1_pressed);
	for(EasyEntity* e : players)
		e->Update(_dt, e != player ? false : mb1_pressed);

	isRender = network->IsInGame();

	if (network->IsInGame())
	{
		NetworkUpdate(_dt);
	}

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

void EasyPlayground::scroll_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (!camera->mode)
	{
		ImGui_ImplGlfw_ScrollCallback(window, xpos, ypos);
		if (ImGui::GetIO().WantCaptureMouse)
			return;
	}

	camera->scroll_callback(xpos, ypos);
}

void EasyPlayground::cursor_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (!camera->mode)
	{
		ImGui_ImplGlfw_CursorPosCallback(window, xpos, ypos);
		if (ImGui::GetIO().WantCaptureMouse)
			return;
	}

	camera->cursor_callback(xpos, ypos);
}

void EasyPlayground::mouse_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (!camera->mode)
	{
		ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
		if (ImGui::GetIO().WantCaptureMouse)
			return;
	}

	if (button == GLFW_MOUSE_BUTTON_1 && action == GLFW_PRESS)
		mb1_pressed = true;
	else if (button == GLFW_MOUSE_BUTTON_1 && action == GLFW_RELEASE)
		mb1_pressed = false;

	camera->mouse_callback(window, button, action, mods);
}

void EasyPlayground::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_GRAVE_ACCENT && action == GLFW_RELEASE)
	{
		isConsoleWindow = !isConsoleWindow;
		return;
	}

	if (key == GLFW_KEY_F1 && action == GLFW_RELEASE)
	{
		isTestRender = !isTestRender;
		if (isTestRender)
		{
			for (auto& p : players)
				delete p;
			players.clear();
			player = new EasyEntity(model);
			players.push_back(player);
		}
		return;
	}

	if (key == GLFW_KEY_LEFT_ALT && action == GLFW_RELEASE)
	{
		camera->ModeSwap(false);
		return;
	}
	else if (key == GLFW_KEY_LEFT_ALT && action == GLFW_PRESS)
	{
		camera->ModeSwap(true);
		return;
	}

	ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
	if (ImGui::GetIO().WantCaptureKeyboard)
		return;

	if (camera->key_callback(key, scancode, action, mods))
	{
		return;
	}

	if (key == GLFW_KEY_ESCAPE && action == GLFW_RELEASE)
	{
		display->exitRequested = true;
		return;
	}

	if (key == GLFW_KEY_T && action == GLFW_RELEASE)
	{
		animation = 2;
		return;
	}

	if (key == GLFW_KEY_W && action == GLFW_RELEASE)
	{
		network->GetSendCache().push_back(new sMoveInput(network->session.uid, 0U, 0U, glm::vec3(0.0f), 0U));
		return;
	}

	if (key == GLFW_KEY_R && action == GLFW_RELEASE)
	{
		ReloadShaders();
		return;
	}

	if (key == GLFW_KEY_P && action == GLFW_RELEASE)
	{
		ReGenerateMap();
		return;
	}
}

void EasyPlayground::char_callback(GLFWwindow* window, unsigned int codepoint) const
{
	if (!camera->mode)
	{
		ImGui_ImplGlfw_CharCallback(window, codepoint);
		if (ImGui::GetIO().WantCaptureKeyboard)
			return;
	}
}

void EasyPlayground::ForwardStandartIO()
{
	using namespace UIConsole;
	gImGuiCoutBuf = new ImGuiConsoleBuf(gConsoleLines);
	gOldCoutBuf = std::cout.rdbuf(gImGuiCoutBuf);
}
