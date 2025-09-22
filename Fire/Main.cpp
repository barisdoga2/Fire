
#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <box2d/box2d.h>
#include <tinyxml2.h>
#include <ozz/animation/offline/raw_animation.h>

#include <glm/glm.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <imgui.h>
#include <assimp/Importer.hpp>
#include <freetype/freetype.h>
#include <curl/curl.h>
#include <cryptopp/sha.h>
#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/local_to_model_job.h>
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/base/maths/simd_math.h>
#include <ozz/base/maths/soa_transform.h>
#include <ozz/base/maths/vec_float.h>
#include <ozz/base/containers/vector.h>
#include <ozz/options/options.h>

#include "Camera.hpp"
#include "Shader.hpp"
#include "Mouse.hpp"
#include "Keyboard.hpp"
#include "Utils.hpp"
#include "Model.hpp"
#include "Animator.hpp"
#include "PlaybackController.hpp"








bool CreateDisplay();

std::string title = "Fire!";
glm::tvec2<int> windowSize = glm::tvec2<int>(800, 600);
GLFWwindow* window;
Shader* model_shader;
Camera* camera;
Entity* entity;
Model* model;

class Listener : public MouseListener, public KeyboardListener {
public:
	bool mouse_callback(GLFWwindow* window, int button, int action, int mods)
	{
		return false;
	}
	bool scroll_callback(GLFWwindow* window, double x, double y)
	{
		return false;
	}
	bool cursorMove_callback(glm::vec2 normalizedDelta, bool checkCursor = true)
	{
		return false;
	}
	bool key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		return false;
	}
	bool character_callback(GLFWwindow* window, unsigned int codepoint)
	{
		return false;
	}
};
Listener listener;

void Update(float deltaMs)
{
	entity->animator->UpdateAnimation(deltaMs / 1000.f);
	entity->CreateTransformationMatrix();

	static unsigned int ctr = 0U;
	if (ctr++ >= 10)
	{
		std::cout << "Camera Position:(" << camera->position.x << ", " << camera->position.y << ", " << camera->position.z << "), Rotation:(" << camera->rotation.x << ", " << camera->rotation.y << ", " << camera->rotation.z << ")" << std::endl;
		ctr = 0U;
	}
}

void Render(float deltaMs)
{
	// Early Render
	{
		// Set Viewport
		glViewport(0, 0, windowSize.x, windowSize.y);

		// Clear Color
		glClearColor(255, 0, 0, 1);

		// Clear Screen
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	// Actual Render
	{
		camera->Update(deltaMs);

		// Prepare
		glEnable(GL_MULTISAMPLE);         // turn on MSAA
		glEnable(GL_FRAMEBUFFER_SRGB);    // if using sRGB framebuffer (optional)

		glEnable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		glDisable(GL_STENCIL_TEST);
		glUseProgram(model_shader->prog);
		model_shader->LoadUniform("viewMatrix", camera->viewMatrix);
		model_shader->LoadUniform("projectionMatrix", camera->projectionMatrix);

		// Bind Entity
		model_shader->LoadUniform("transformationMatrix", entity->transformationMatrix);
		if (entity->animator != nullptr)
		{
			if (true)
			{
				const std::vector<glm::mat4>& boneMatrices = entity->animator->GetFinalBoneMatrices();
				for (int i = 0; i < MAX_BONES; i++)
					model_shader->LoadUniform(std::string(std::string("jointTransforms[") + std::to_string(i).c_str() + std::string("]")).c_str(), boneMatrices.at(i));
			}
			else
			{
				const ozz::vector<ozz::math::Float4x4>& boneMatricesOZZ = entity->animator->models_;
				for (int i = 0; i < boneMatricesOZZ.size(); i++)
					model_shader->LoadUniform(std::string(std::string("jointTransforms[") + std::to_string(i).c_str() + std::string("]")).c_str(), (const glm::mat4x4&)boneMatricesOZZ.at(i));

				glm::mat4 ident(1.0f);
				for (int i = 0; i < 200U - boneMatricesOZZ.size(); i++)
					model_shader->LoadUniform(std::string(std::string("jointTransforms[") + std::to_string(boneMatricesOZZ.size() + i).c_str() + std::string("]")).c_str(), ident);
			}
		}

		// Bind Meshes
		for (Mesh* mesh : entity->model->meshes)
		{
			glBindVertexArray(mesh->VAO);
			glEnableVertexAttribArray(0);
			glEnableVertexAttribArray(1);
			glEnableVertexAttribArray(2);
			glEnableVertexAttribArray(3);
			glEnableVertexAttribArray(4);
			glEnableVertexAttribArray(5);

			// Bind Texture
			model_shader->BindTexture(0, GL_TEXTURE_2D, "albedoT", mesh->albedoT);

			// Render Mesh Instances
			for (Transformable instance : mesh->instances)
			{
				model_shader->LoadUniform("meshTransformationMatrix", instance.transformationMatrix);
				glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(mesh->indices.size()), GL_UNSIGNED_INT, 0);
			}
			glBindVertexArray(0);;
		}

	}

	// End Render
	{
		{
			if (false)
			{
				{
					const ts_t f_prd = 100ULL;
					static ts_t f_ctr = 0ULL;

					f_ctr++;

					ts_t ts = GetCurrentMs();
					static ts_t f_tmr = ts;

					ts_t diff = ts - f_tmr;
					if (diff >= f_prd)
					{
						double scaled_fps = (f_ctr * 1000.0) / (double)diff;
						//glfwSetWindowTitle(window, std::string(title + " | FPS: " + std::to_string(scaled_fps).c_str()).c_str());
						f_tmr = ts;
						f_ctr = 0;
					}
				}

				{
					const ts_t f_prd = 100ULL;
					static ts_t f_ctr = 0ULL;

					f_ctr++;

					ts_t ts = GetCurrentNs();
					static ts_t f_tmr = ts;

					ts_t diff = ts - f_tmr;
					if (diff >= f_prd * 1000000ULL)
					{
						double scaled_fps = (f_ctr * 1000.0) / (double)(diff / 1000000ULL);
						//glfwSetWindowTitle(window, std::string(title + " | FPS: " + std::to_string(scaled_fps).c_str()).c_str());
						f_tmr = ts;
						f_ctr = 0;
					}
				}

				{
					ts_t ts = GetCurrentMs();
					static const unsigned int n_frames = 10U;
					static ts_t frames[n_frames] = { ts };
					static unsigned int frames_idx = 0U;

					ts_t ts_newest = frames[frames_idx];
					ts_t ts_oldest = frames[(frames_idx + 1U) % n_frames];

					double diff = (double)(ts_oldest - ts_newest);

					frames[frames_idx] = ts;
					frames_idx = (++frames_idx) % n_frames;

					double scaled_fps = (1000.0 * n_frames) / diff;
					//glfwSetWindowTitle(window, std::string(title + " | FPS: " + std::to_string(scaled_fps).c_str()).c_str());
				}
			}
		}

		// Calculate FPS and Update Title
		float sample = GetCurrentMillis();
		static float frameCtr = 0;
		static float lastSample = sample;
		if (sample - lastSample >= 1000)
		{
			glfwSetWindowTitle(window, std::string(title + " | FPS: " + std::to_string(frameCtr).c_str()).c_str());
			lastSample = sample;
			frameCtr = 0;
		}
		frameCtr++;

		// Update Display and Get Events
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
}

bool Init()
{
	// Initialize Display
	if (!CreateDisplay())
		return false;

	// Initialize Camera
	camera = new Camera(glm::vec3(0, 170.571, 76.5048), glm::vec3(0, 0, 0), 90.0f, windowSize, 0.1f, 1000.0f);

	// Initialize Inputs
	Mouse::Init(window);
	Keyboard::Init(window);
	Mouse::AddListener(&listener);
	Keyboard::AddListener(&listener);
	Mouse::AddListener(camera);
	Keyboard::AddListener(camera);
	std::cout << "F" << std::endl;

	// Initialize Shaders
	{
		model_shader = new Shader("model", { "position", "textureCoords", "normal", "tangent", "jointIndices", "weights" });

		model_shader->ConnectTextureUnits(std::vector<std::string>{"albedoT"});
		model_shader->ConnectUniforms(
			std::vector<std::string>{
			"transformationMatrix"
				, "meshTransformationMatrix"
				, "projectionMatrix"
				, "viewMatrix"
		}
		);

		model_shader->ConnectUniformArray("jointTransforms", MAX_BONES);
	}

	// Initialize Model, Entity, Animator
	{
		model = LoadModel("TPose.fbx");
		entity = new Entity(model);
		entity->animator = new Animator((*model->animations.begin()).second, (*model->ozzAnimations.begin()).second);
	}

	return true;
}

bool CreateDisplay()
{
	if (!glfwInit())
		return false;

	// Get Primary Monitor and Get Monitor size
	GLFWmonitor* primary = glfwGetPrimaryMonitor();
	const GLFWvidmode* mode = glfwGetVideoMode(primary);
	glm::tvec2<int> monitorSize = glm::tvec2<int>(mode->width, mode->height);

	// Other Window Hints
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	//glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE); // MacOS ?
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	//glfwWindowHint(GLFW_DECORATED, NULL); // Remove the border and titlebar.. 
	//glfwWindowHint(GLFW_SCALE_TO_MONITOR, ???); // DPI Scaling
	glfwWindowHint(GLFW_SAMPLES, 4); // request 4x MSAA

	// Craete The Window
	window = glfwCreateWindow(windowSize.x, windowSize.y, title.c_str(), NULL, NULL);

	// Check If OK
	if (!window)
	{
		std::cout << "GLFW Window couldn't created" << std::endl;
		glfwTerminate();
		return false;
	}

	// Center The Window
	glfwSetWindowPos(window, (int)((monitorSize.x - windowSize.x) / 2), 30);

	// Enable GL Context
	glfwMakeContextCurrent(window);

	// VSync
	glfwSwapInterval(false);

	// Get Frame Buffer Size
	//glfwGetFramebufferSize(window, &fboSize.x, &fboSize.y);

	// Final Check
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return false;
	}

	// Add Resize Link
	//glfwSetWindowSizeCallback(window, DisplayManager::DisplayResized);

	return true;
}

int main()
{
	if (!Init())
		return -1;

	// Main Loop
	{
		float renderResolution = 1000.f / glfwGetVideoMode(glfwGetPrimaryMonitor())->refreshRate;
		float updateResolution = 1000.f / 24.f;
		float renderTimer = 0;
		float updateTimer = 0;
		float lastTs = GetCurrentMillis();
		while (!glfwWindowShouldClose(window))
		{
			float currentTs = GetCurrentMillis();
			float deltaTs = currentTs - lastTs;
			lastTs = currentTs;

			renderTimer += deltaTs;
			updateTimer += deltaTs;

			if (renderTimer >= renderResolution)
			{
				Render(renderTimer);
				renderTimer = 0;
			}

			if (updateTimer >= updateResolution)
			{
				Update(updateTimer);
				updateTimer = 0;
			}

		}
	}
	
	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}