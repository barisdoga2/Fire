#include "EasyPlayground.hpp"
#include "EasyMaterial.hpp"
#include "HDR.hpp"
#include "GL_Ext.hpp"
#include "Config.hpp"

#include <iostream>
#include <chrono>
#include <sstream>
#include <iomanip>

#include <glm/gtc/type_ptr.hpp>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_internal.h>
#include "imgui_impl_opengl3.h"
#include "imgui_impl_glfw.h"
#include "EasyUtility.hpp"

#include "ImGui_Fun.hpp"


EasyPlayground::EasyPlayground(const EasyDisplay& display, EasyBufferManager* bm) : display_(display), bm(bm), network(bm)
{

}

EasyPlayground::~EasyPlayground()
{
	network.Stop();

	// ImGUI
	{
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}
}

bool LoadFileBinary(std::string file, std::vector<char>* out)
{
	// Create the stream
	std::ifstream infile(file, std::ios::binary | std::ios::ate);

	// Check if file opened
	if (infile.good())
	{
		// Get length of the file
		size_t length = (size_t)infile.tellg();

		// Move to begining
		infile.seekg(0, std::ios::beg);

		// Resize the vector
		out->resize(length * sizeof(char));

		// Read into the buffer
		infile.read(out->data(), length);

		// Close the stream
		infile.close();

		return true;
	}
	else
	{
		return false;
	}
}

bool EasyPlayground::Init()
{
	// Callbacks
	{
		static EasyPlayground* instance = this;
		glfwSetKeyCallback(display_.window, [](GLFWwindow* w, int k, int s, int a, int m) { instance->key_callback(w, k, s, a, m); });
		glfwSetCursorPosCallback(display_.window, [](GLFWwindow* w, double x, double y) { instance->cursor_callback(w, x, y); });
		glfwSetMouseButtonCallback(display_.window, [](GLFWwindow* window, int button, int action, int mods) { instance->mouse_callback(window, button, action, mods); });
		glfwSetScrollCallback(display_.window, [](GLFWwindow* w, double x, double y) { instance->scroll_callback(w, x, y); });
		glfwSetCharCallback(display_.window, [](GLFWwindow* w, unsigned int x) { instance->char_callback(w, x); });
	}

	// Asset
	{
		mapObjects.push_back(walls);
	}

	// Shader
	{
		ReloadShaders();

		//EasyMaterial* back = new EasyMaterial("terrainGrass");
		//EasyMaterial* r = new EasyMaterial("terrainMud");
		//EasyMaterial* g = new EasyMaterial("terrainPath");
		//EasyMaterial* b = new EasyMaterial("terrainBrick");

		//std::vector<char> blend, height;
		//LoadFileBinary(GetPath("res/images/blend.png"), &blend);
		//LoadFileBinary(GetPath("res/images/height.png"), &height);


		ReGenerateMap();
	}

	// ImGUI
	{
		const char* glsl_version = "#version 130";

		// Setup Dear ImGui context
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

		// Setup Dear ImGui style
		ImGui::StyleColorsDark();
		//ImGui::StyleColorsLight();

		// Setup Platform/Renderer backends
		ImGui_ImplGlfw_InitForOpenGL(display_.window, false);
		ImGui_ImplOpenGL3_Init(glsl_version);
	}

	char usernameArr[16] = "";
	char passwordArr[32] = "";
	LoadConfig(rememberMe, usernameArr, passwordArr);
	network.session.username = usernameArr;
	network.session.password = passwordArr;

	return true;
}

bool EasyPlayground::Render(double _dt)
{
	//if (model->animator)
	//	model->animator->BlendTo(model->animations.at(animation ? 1 : 0), 1.0);

	model->Update(_dt, mb1_pressed);
	cube_1x1x1->Update(_dt);

	camera.Update(_dt);

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
	if (isRender)
	{
		{
			std::vector<EasyModel*> objs = { model };
			objs.insert(objs.end(), mapObjects.begin(), mapObjects.end());

			SkyboxRenderer::Render(camera);

			if (imgui_showNormalLines)
			{
				normalLinesShader.Start();

				normalLinesShader.LoadUniform("view", camera.view_);
				normalLinesShader.LoadUniform("proj", camera.projection_);
				normalLinesShader.LoadUniform("normalLength", imgui_showNormalLength);

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

						for (EasyModel::EasyTransform* t : kv.second)
						{
							normalLinesShader.LoadUniform("model",
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

				for (Chunk* chunk : chunks)
				{
					glBindVertexArray(chunk->VAO);
					glEnableVertexAttribArray(0);
					normalLinesShader.LoadUniform("model", glm::translate(glm::mat4x4(1), glm::vec3(chunk->coord.x * Chunk::CHUNK_SIZE, 0, chunk->coord.y * Chunk::CHUNK_SIZE)));

					glDrawElements(GL_TRIANGLES, (GLint)chunk->indices.size(), GL_UNSIGNED_INT, 0);

					glDisableVertexAttribArray(0);
					glBindVertexArray(0);
				}

				normalLinesShader.Stop();

			}

			if (imgui_triangles)
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

			{
				shader.Start();

				shader.LoadUniform("view", camera.view_);
				shader.LoadUniform("proj", camera.projection_);
				shader.LoadUniform("uCameraPos", camera.position);
				shader.LoadUniform("uIsFog", imgui_isFog ? 1.0f : 0.0f);

				// Render Players
				{
					if (model->animator)
						shader.LoadUniform("boneMatrices", model->animator->GetFinalBoneMatrices());

					//for (auto& [uid, networkPlayer] : networkPlayers)
					//{
					//	for (const auto& kv : model->instances)
					//	{
					//		EasyModel::EasyMesh* mesh = kv.first;

					//		if (!mesh->LoadToGPU())
					//			continue;

					//		GL(BindVertexArray(mesh->vao));
					//		GL(EnableVertexAttribArray(0));
					//		GL(EnableVertexAttribArray(1));
					//		GL(EnableVertexAttribArray(2));
					//		GL(EnableVertexAttribArray(3));
					//		GL(EnableVertexAttribArray(4));
					//		GL(EnableVertexAttribArray(5));
					//		GL(EnableVertexAttribArray(6));

					//		glActiveTexture(GL_TEXTURE0);
					//		glBindTexture(GL_TEXTURE_2D, mesh->texture);
					//		shader.LoadUniform("diffuse", 0);

					//		shader.LoadUniform("animated", mesh->animatable ? 1 : 0);

					//		for (EasyModel::EasyTransform* t : kv.second)
					//		{
					//			shader.LoadUniform("model", CreateTransformMatrix(t->position + networkPlayer.transform.position, t->rotationQuat, t->scale));
					//			glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(mesh->indices.size()), GL_UNSIGNED_INT, 0);
					//		}

					//		GL(DisableVertexAttribArray(6));
					//		GL(DisableVertexAttribArray(5));
					//		GL(DisableVertexAttribArray(4));
					//		GL(DisableVertexAttribArray(3));
					//		GL(DisableVertexAttribArray(2));
					//		GL(DisableVertexAttribArray(1));
					//		GL(DisableVertexAttribArray(0));
					//		GL(BindVertexArray(0));
					//	}
					//}

				}
				
				for (EasyModel* model : objs)
				{
					if (model->animator)
						shader.LoadUniform("boneMatrices", model->animator->GetFinalBoneMatrices());

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
						shader.LoadUniform("diffuse", 0);

						shader.LoadUniform("animated", mesh->animatable ? 1 : 0);

						for (EasyModel::EasyTransform* t : kv.second)
						{
							shader.LoadUniform("model", CreateTransformMatrix(t->position, t->rotationQuat, t->scale));
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

				shader.Stop();
			}

			if (imgui_triangles)
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}

		{
			if (imgui_triangles)
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

			ChunkRenderer::Render(&camera, chunks, hdr, imgui_isFog);

			if (imgui_triangles)
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}
	}

	ImGUIRender();

	return success;
}

void EasyPlayground::OnDisconnect(SessionStatus disconnectStatus)
{
	//network.isLogin = false;
}

void EasyPlayground::OnLogin()
{
	//network.isLogin = true;

	//network.lastHeartbeatReceive = Clock::now();
	//network.nextHeartbeatSend = Clock::now();
	//network.nextPlayerMovementSend = Clock::now();
}

void EasyPlayground::NetworkUpdate(double _dt)
{
	network.Update();
	//if (!network.isLogin)
	//	return;
	//for (auto it = network.in_cache.begin(); it != network.in_cache.end(); )
	//{
	//	if (auto* d = dynamic_cast<sDisconnectResponse*>(*it); d)
	//	{
	//		std::cout << "Disconnect response received!\n";
	//		loggedIn = false;
	//		loginFailed = true;
	//		loginStatusText = d->message;
	//		delete d;
	//		it = network.in_cache.erase(it);
	//	}
	//	else if (auto* h = dynamic_cast<sHearbeat*>(*it); h)
	//	{
	//		std::cout << "Heartbeat received!\n";
	//		network.lastHeartbeatReceive = Clock::now();
	//		delete h;
	//		it = network.in_cache.erase(it);
	//	}
	//	else if (auto* b = dynamic_cast<sBroadcastMessage*>(*it); b)
	//	{
	//		std::cout << "Broadcast message received!\n";
	//		isBroadcastMessage = true;
	//		broadcastMessage = b->message;
	//		delete b;
	//		it = network.in_cache.erase(it);
	//	}
	//	else if (auto* c = dynamic_cast<sChatMessage*>(*it); c)
	//	{
	//		std::cout << "Chat message received!\n";
	//		OnChatMessageReceived(c->username, c->message, c->timestamp);
	//		delete b;
	//		it = network.in_cache.erase(it);
	//	}
	//	else if (auto* m = dynamic_cast<sPlayerMovementPack*>(*it); m)
	//	{
	//		std::cout << "Player movement pack received!\n";

	//		// Handle Packet
	//		for (sPlayerMovement& p : m->movements)
	//		{
	//			//p.userID; // user specifier
	//			//p.position; // position at packet creation
	//			//p.rotation; // rotation at packet creation
	//			//p.direction; // direction at packet creation
	//			//unsigned long long timestamp; // time on packet creation. Im working on lan, so now think the server and clients are perfectly time sync and this timestamp inserted when the packet is created. So server collects the sPlayerMovement packets and redirect to each other client with packing into one packet. 
	//			if (p.userID != client.client.user_id)
	//			{
	//				// Other players data
	//				if (auto res = networkPlayers.find(p.userID); res != networkPlayers.end())
	//				{
	//					res->second.transform.position = p.position;
	//				}
	//				else
	//				{
	//					networkPlayers.insert({ p.userID, {p.userID, "", {{p.position}, {0,0,0}, {1,1,1}}} });
	//				}
	//			}
	//			else
	//			{
	//				// This players data, ignore
	//			}
	//		}
	//		delete m;
	//		it = network.in_cache.erase(it);
	//	}
	//	else
	//	{
	//		++it;
	//	}
	//}
	//if (network.lastHeartbeatReceive + std::chrono::seconds(10) < Clock::now())
	//{
	//	std::cout << "Disconnect reason: '" + SessionStatus_Str(SERVER_TIMED_OUT) + "'!" + "\n";
	//	loggedIn = false;
	//	loginFailed = true;
	//	loginStatusText = "Disconnect reason: '" + SessionStatus_Str(SERVER_TIMED_OUT) + "'!";
	//}
	//if (network.nextHeartbeatSend < Clock::now())
	//{
	//	network.nextHeartbeatSend += std::chrono::seconds(1);
	//	network.out_cache.push_back(new sHearbeat());
	//	std::cout << "Heartbeat sent!\n";
	//}
	//if (network.nextPlayerMovementSend < Clock::now())
	//{
	//	network.nextPlayerMovementSend += std::chrono::seconds(1);
	//	position.x += 0.01f;
	//	rotation.x += 0.02f;
	//	direction.x += 0.04f;
	//	moveTimestamp += 1000;
	//	network.out_cache.push_back(new sPlayerMovement(stats.uid, position, rotation, direction, moveTimestamp));
	//	std::cout << "Player movement sent!\n";
	//}
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

	//if (network.m.try_lock())
	//{
	//	network.in_cache.insert(network.in_cache.end(), network.in.begin(), network.in.end());
	//	network.in.clear();
	//	network.out.insert(network.out.end(), network.out_cache.begin(), network.out_cache.end());
	//	network.out_cache.clear();
	//	network.m.unlock();
	//}

	NetworkUpdate(_dt);

	return !exitRequested;
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

void EasyPlayground::EndRender()
{
	glfwSetWindowTitle(display_.window, (std::ostringstream() << std::fixed << std::setprecision(3) << "FPS: " << fps << " | UPS: " << ups).str().c_str());
	glfwSwapBuffers(display_.window);
	glfwPollEvents();
}

void EasyPlayground::ReloadShaders()
{
	normalLinesShader = EasyShader("NormalLines");
	normalLinesShader.Start();
	normalLinesShader.BindAttribs({ "aPos", "aUV", "aNormal" });
	normalLinesShader.BindUniforms({ "model", "view", "proj", "normalLength" });
	normalLinesShader.Stop();

	shader = EasyShader("model");
	shader.Start();
	shader.BindAttribs({ "position", "uv", "normal", "tangent", "bitangent", "boneIds", "weights" });
	shader.BindUniforms(GENERAL_UNIFORMS);
	shader.BindUniforms({ "diffuse", "view", "proj", "model", "animated" });
	shader.BindUniformArray("boneMatrices", 200);
	shader.Stop();

	SkyboxRenderer::Init();
	HDR::Init();
	hdr = new HDR("defaultLightingHDR");
	ChunkRenderer::Init();
}

void EasyPlayground::ReGenerateMap()
{
	// Cleanup
	for (size_t i = 0u; i < chunks.size(); i++)
		delete chunks.at(i);
	chunks.clear();

	// New Seed
	seed = (int)(glm::linearRand(0.7f, 1.3f) * 10000u);

	// Generate Center
	Chunk::GenerateChunkAt(chunks, { 0,0 }, seed);

	// Make 3x3
	//Chunk::GenerateChunkAt(chunks, { 1,0 }, seed);
	//Chunk::GenerateChunkAt(chunks, { -1,0 }, seed);
	//Chunk::GenerateChunkAt(chunks, { 0,1 }, seed);
	//Chunk::GenerateChunkAt(chunks, { 0,-1 }, seed);
	//Chunk::GenerateChunkAt(chunks, { 1,1 }, seed);
	//Chunk::GenerateChunkAt(chunks, { 1,-1 }, seed);
	//Chunk::GenerateChunkAt(chunks, { -1,1 }, seed);
	//Chunk::GenerateChunkAt(chunks, { -1,-1 }, seed);



#if 1
	// Generate some other

#endif

#if 0
	// Add lake
	Chunk::AddLake(chunks, glm::vec2(0.f, 0.f), 30.0f);   // small pond
	Chunk::AddLake(chunks, glm::vec2(100.f, 0.f), 60.0f); // big lake
#endif

	// Load all to GPU
	for (Chunk* c : chunks)
		c->LoadToGPU();
}

void EasyPlayground::scroll_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (!camera.mode)
	{
		ImGui_ImplGlfw_ScrollCallback(window, xpos, ypos);
		if (ImGui::GetIO().WantCaptureMouse)
			return;
	}

	camera.scroll_callback(xpos, ypos);
}

void EasyPlayground::cursor_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (!camera.mode)
	{
		ImGui_ImplGlfw_CursorPosCallback(window, xpos, ypos);
		if (ImGui::GetIO().WantCaptureMouse)
			return;
	}

	camera.cursor_callback(xpos, ypos);
}

void EasyPlayground::mouse_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (!camera.mode)
	{
		ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
		if (ImGui::GetIO().WantCaptureMouse)
			return;
	}

	if (button == GLFW_MOUSE_BUTTON_1 && action == GLFW_PRESS)
		mb1_pressed = true;
	else if (button == GLFW_MOUSE_BUTTON_1 && action == GLFW_RELEASE)
		mb1_pressed = false;

	camera.mouse_callback(window, button, action, mods);
}

void EasyPlayground::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_F1 && action == GLFW_RELEASE)
	{
		isRender = true;
		return;
	}

	if (key == GLFW_KEY_LEFT_ALT && action == GLFW_RELEASE)
	{
		camera.ModeSwap(false);
		return;
	}
	else if (key == GLFW_KEY_LEFT_ALT && action == GLFW_PRESS)
	{
		camera.ModeSwap(true);
		return;
	}

	ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
	if (ImGui::GetIO().WantCaptureKeyboard)
		return;

	if (camera.key_callback(key, scancode, action, mods))
	{
		return;
	}

	if (key == GLFW_KEY_ESCAPE && action == GLFW_RELEASE)
	{
		exitRequested = 1;
		return;
	}

	if (key == GLFW_KEY_T && action == GLFW_RELEASE)
	{
		animation = 2;
		return;
	}

	static glm::ivec2 ps{};
	if (key == GLFW_KEY_S && action == GLFW_RELEASE)
	{
		ps.y--;
		if (Chunk* c = Chunk::GenerateChunkAt(chunks, ps, seed); c)
			c->LoadToGPU();
		return;
	}
	if (key == GLFW_KEY_W && action == GLFW_RELEASE)
	{
		ps.x--;
		if (Chunk* c = Chunk::GenerateChunkAt(chunks, ps, seed); c)
			c->LoadToGPU();
		return;
	}
	if (key == GLFW_KEY_N && action == GLFW_RELEASE)
	{
		ps.y++;
		if (Chunk* c = Chunk::GenerateChunkAt(chunks, ps, seed); c)
			c->LoadToGPU();
		return;
	}
	if (key == GLFW_KEY_E && action == GLFW_RELEASE)
	{
		ps.x++;
		if (Chunk* c = Chunk::GenerateChunkAt(chunks, ps, seed); c)
			c->LoadToGPU();
		return;
	}


	if (key == GLFW_KEY_R && action == GLFW_RELEASE)
	{
		std::cout << "Reloading shaders...\n";
		ReloadShaders();
		return;
	}

	if (key == GLFW_KEY_P && action == GLFW_RELEASE)
	{
		std::cout << "Regenerating map...\n";
		ReGenerateMap();
		return;
	}
}

void EasyPlayground::char_callback(GLFWwindow* window, unsigned int codepoint)
{
	if (!camera.mode)
	{
		ImGui_ImplGlfw_CharCallback(window, codepoint);
		if (ImGui::GetIO().WantCaptureKeyboard)
			return;
	}
}