#include "EasyPlayground.hpp"
#include "GL_Ext.hpp"

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



EasyPlayground::EasyPlayground(const EasyDisplay& display) : display_(display)
{
	
}

EasyPlayground::~EasyPlayground()
{
	// ImGUI
	{
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
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
		model.LoadToGPU();
	}

	// Shader
	{
		shader.Start();
		shader.BindAttribs({ "position", "uv", "normal", "tangent", "bitangent", "boneIds", "weights" });
		shader.BindUniforms({ "diffuse", "view", "proj", "model", "animated" });
		shader.BindUniformArray("boneMatrices", 200);
		shader.Stop();
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
		io.Fonts->AddFontFromFileTTF("../../res/Arial.ttf", 20.f, 0, ranges.Data);
		io.Fonts->Build();

		// Setup Dear ImGui style
		ImGui::StyleColorsDark();
		//ImGui::StyleColorsLight();

		// Setup Platform/Renderer backends
		ImGui_ImplGlfw_InitForOpenGL(display_.window, false);
		ImGui_ImplOpenGL3_Init(glsl_version);
	}

    return true;
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

	GL(ClearDepth(1.f));
	GL(ClearColor(.4f, .42f, .38f, 1.f));
	GL(Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
	
	// Setup default states
	GL(Disable(GL_CULL_FACE)); //GL(Enable(GL_CULL_FACE));
	GL(CullFace(GL_BACK));
	GL(Enable(GL_DEPTH_TEST));
	GL(DepthMask(GL_TRUE));
	GL(DepthFunc(GL_LEQUAL));
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

	// Rendering
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void EasyPlayground::EndRender()
{
	glfwSetWindowTitle(display_.window, (std::ostringstream() << std::fixed << std::setprecision(3) << "FPS: " << fps << " | UPS: " << ups).str().c_str());
	glfwSwapBuffers(display_.window);
	glfwPollEvents();
}

bool EasyPlayground::Render(double _dt)
{
	//if (model.animator)
	//	model.animator->BlendTo(model.animations.at(animation ? 1 : 0), 1.0);

	model.Update(_dt, mb1_pressed);

	camera.Update(_dt);

	bool success = true;

	// Render
	{
		shader.Start();

		shader.LoadUniform("view", camera.view_);
		shader.LoadUniform("proj", camera.projection_);

		std::vector<EasyModel*> models = { &model };
		for (const EasyModel* m : models)
		{

			shader.LoadUniform("model", CreateTransformMatrix({ 0, 0, 0 }, { 0,0,0 }, { 1 ,1 ,1 }));

			for (const EasyModel::EasyMesh* mesh : m->meshes)
			{
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

				if (m->animator)
				{
					shader.LoadUniform("boneMatrices", m->animator->GetFinalBoneMatrices());
					shader.LoadUniform("animated", 1);
				}
				else
				{
					shader.LoadUniform("animated", 0);
				}
				glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(mesh->indices.size()), GL_UNSIGNED_INT, 0);

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

	return success;
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
	if (!camera.mode)
	{
		ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
		if (ImGui::GetIO().WantCaptureKeyboard)
			return;
	}

	camera.key_callback(key, scancode, action, mods);

	if (key == GLFW_KEY_ESCAPE && action == GLFW_RELEASE)
	{
		exitRequested = 1;
	}

	if (key == GLFW_KEY_T && action == GLFW_RELEASE)
	{
		animation = 2;
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