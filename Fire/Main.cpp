#ifdef MAIN
#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <box2d/box2d.h>
#include <tinyxml2.h>
#include <ozz/animation/offline/raw_animation.h>

#include <glm/glm.hpp>
#include <glm/gtx/type_trait.hpp>
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
#include <ozz/base/memory/allocator.h>
#include <ozz/base/maths/simd_math.h>
#include <ozz/base/maths/soa_transform.h>
#include <ozz/base/maths/vec_float.h>
#include <ozz/base/containers/vector.h>
#include <ozz/options/options.h>
#include <ozz/geometry/runtime/skinning_job.h>
#include <ozz/base/maths/internal/simd_math_sse-inl.h>

#include "Camera.hpp"
#include "Shader.hpp"
#include "Mouse.hpp"
#include "Keyboard.hpp"
#include "Utils.hpp"
#include "Model.hpp"
#include "Animator.hpp"
#include "PlaybackController.hpp"
#include "mesh.h"

#define GL_PTR_OFFSET(i) reinterpret_cast<void*>(static_cast<intptr_t>(i))

#define GL(_f)                                                     \
  do {                                                             \
    gl##_f;                                                        \
    GLenum gl_err = glGetError();                                  \
    if (gl_err != 0) {                                             \
      ozz::log::Err() << "GL error 0x" << std::hex << gl_err       \
                      << " returned from 'gl" << #_f << std::endl; \
    }                                                              \
    assert(gl_err == GL_NO_ERROR);                                 \
  } while (void(0), 0)

namespace ozz {
	namespace sample {
		static bool LoadSkeleton(const char* _filename, ozz::animation::Skeleton* _skeleton) {
			assert(_filename && _skeleton);
			ozz::log::Out() << "Loading skeleton archive " << _filename << "."
				<< std::endl;
			ozz::io::File file(_filename, "rb");
			if (!file.opened()) {
				ozz::log::Err() << "Failed to open skeleton file " << _filename << "."
					<< std::endl;
				return false;
			}
			ozz::io::IArchive archive(&file);
			if (!archive.TestTag<ozz::animation::Skeleton>()) {
				ozz::log::Err() << "Failed to load skeleton instance from file "
					<< _filename << "." << std::endl;
				return false;
			}

			{
				archive >> *_skeleton;
			}
			return true;
		}

		static bool LoadAnimation(const char* _filename,
			ozz::animation::Animation* _animation) {
			assert(_filename && _animation);
			ozz::log::Out() << "Loading animation archive: " << _filename << "."
				<< std::endl;
			ozz::io::File file(_filename, "rb");
			if (!file.opened()) {
				ozz::log::Err() << "Failed to open animation file " << _filename << "."
					<< std::endl;
				return false;
			}
			ozz::io::IArchive archive(&file);
			if (!archive.TestTag<ozz::animation::Animation>()) {
				ozz::log::Err() << "Failed to load animation instance from file "
					<< _filename << "." << std::endl;
				return false;
			}

			{
				archive >> *_animation;
			}

			return true;
		}

		static bool LoadMeshes(const char* _filename,
			ozz::vector<ozz::sample::Mesh>* _meshes) {
			assert(_filename && _meshes);
			ozz::log::Out() << "Loading meshes archive: " << _filename << "."
				<< std::endl;
			ozz::io::File file(_filename, "rb");
			if (!file.opened()) {
				ozz::log::Err() << "Failed to open mesh file " << _filename << "."
					<< std::endl;
				return false;
			}
			ozz::io::IArchive archive(&file);

			{
				while (archive.TestTag<ozz::sample::Mesh>()) {
					_meshes->resize(_meshes->size() + 1);
					archive >> _meshes->back();
				}
			}

			return true;
		}

		class PlaybackController {
		public:
			PlaybackController() : time_ratio_(0.f), previous_time_ratio_(0.f), playback_speed_(1.f), play_(true), loop_(true)
			{

			}

			int set_time_ratio(float _ratio)
			{
				previous_time_ratio_ = time_ratio_;
				if (loop_)
				{
					const float loops = floorf(_ratio);
					time_ratio_ = _ratio - loops;
					return static_cast<int>(loops);
				}
				else
				{
					time_ratio_ = math::Clamp(0.f, _ratio, 1.f);
					return 0;
				}
			}

			float time_ratio() const { return time_ratio_; }

			float previous_time_ratio() const { return previous_time_ratio_; }

			void set_playback_speed(float _speed) { playback_speed_ = _speed; }

			float playback_speed() const { return playback_speed_; }

			void set_loop(bool _loop) { loop_ = _loop; }

			bool loop() const { return loop_; }

			bool playing() const { return play_; }

			int Update(const animation::Animation& _animation, float _dt)
			{
				float new_ratio = time_ratio_;

				if (play_) {
					new_ratio = time_ratio_ + _dt * playback_speed_ / _animation.duration();
				}

				return set_time_ratio(new_ratio);
			}

			void Reset()
			{
				previous_time_ratio_ = time_ratio_ = 0.f;
				playback_speed_ = 1.f;
				play_ = true;
			}

		private:
			float time_ratio_;
			float previous_time_ratio_;
			float playback_speed_;
			bool play_;
			bool loop_;
		};

	}
}

class Listener {
public:
	virtual bool key_callback(int key, int scancode, int action, int mods) { return false; }
	virtual bool character_callback(unsigned int codepoint) { return false; }
};

class Display {
public:
    glm::tvec2<int> windowSize = glm::tvec2<int>(800, 600);
    GLFWwindow* window;

    bool Init()
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
        window = glfwCreateWindow(windowSize.x, windowSize.y, "Hi", NULL, NULL);

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

		return true;
    }

};

class KeyboardV2 {
private:
	struct KeyContext {
		int keys[MAX_KEYS];
		bool keys_read[MAX_KEYS];
		std::vector<Listener*> listeners;
	};

	KeyboardV2() {}

	static KeyContext& GetContext()
	{
		static KeyContext context;
		return context;
	}

	static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		KeyContext& context = GetContext();

		if (key < MAX_KEYS)
			context.keys[key] = action;

		for (Listener* listener : context.listeners)
			if (listener->key_callback(key, scancode, action, mods))
				break;

	}

	static void character_callback(GLFWwindow* window, unsigned int codepoint)
	{
		KeyContext& context = GetContext();

		for (Listener* listener : context.listeners)
			if (listener->character_callback(codepoint))
				break;
	}
public:
	static void Init(const Display& display)
	{
		GetContext();
		glfwSetKeyCallback(display.window, key_callback);
		glfwSetCharCallback(display.window, character_callback);
	}

	static void AddListener(Listener* listener)
	{
		KeyContext& ctx = GetContext();
		if (auto res = std::find(ctx.listeners.begin(), ctx.listeners.end(), listener); res == ctx.listeners.end())
			ctx.listeners.push_back(listener);
	}

	static void RemoveListener(Listener* listener)
	{
		KeyContext& ctx = GetContext();
		if (auto res = std::find(ctx.listeners.begin(), ctx.listeners.end(), listener); res != ctx.listeners.end())
			ctx.listeners.erase(res);
	}
};

class MyMesh {
public:
	class MyVertex {
	public:
		glm::vec3 Position;
		glm::vec2 TexCoords;
		glm::vec3 Normal;
		glm::vec3 Tangent;
		glm::uvec4 BoneIDs;
		glm::vec4 Weights;
	};
	std::vector<MyVertex> vertices;

	std::vector<GLuint> indices;
	GLuint vao, vbo, ebo;
	size_t joint_remaps_size;

	MyMesh(const ozz::sample::Mesh& mesh)
	{
		indices = std::vector<GLuint>(mesh.triangle_indices.begin(), mesh.triangle_indices.end());

		for (const ozz::sample::Mesh::Part& part : mesh.parts)
		{
			auto v_cnt = part.vertex_count();
			auto i_cnt = part.influences_count();
			for (size_t v = 0; v < v_cnt; v++)
			{
				MyVertex vertex;

				vertex.Position = { part.positions.at(v * 3 + 0), part.positions.at(v * 3 + 1), part.positions.at(v * 3 + 2) };
				vertex.TexCoords = { part.uvs.at(v * 2 + 0), part.uvs.at(v * 2 + 1) };
				vertex.Normal = { part.normals.at(v * 3 + 0), part.normals.at(v * 3 + 1), part.normals.at(v * 3 + 2) };
				vertex.Tangent = { part.tangents.at(v * 3 + 0), part.tangents.at(v * 3 + 1), part.tangents.at(v * 3 + 2) };

				glm::uvec4 boneIDs(0.0f);
				for (int j = 0; j < i_cnt && j < 4; ++j)
					boneIDs[j] = part.joint_indices.at(v * i_cnt + j);
				vertex.BoneIDs = boneIDs;

				glm::vec4 weights(0.0f);
				float sum = 0.0f;
				for (int j = 0; j < i_cnt - 1 && j < 4; ++j)
				{
					float w = part.joint_weights[v * (i_cnt - 1) + j];
					weights[j] = w;
					sum += w;
				}
				if (i_cnt > 0 && i_cnt <= 4)
					weights[i_cnt - 1] = 1.0f - sum;
				else if (i_cnt == 0)
					weights[0] = 1.0f;
				vertex.Weights = weights;

				vertices.push_back(vertex);
			}
		}


		GL(GenVertexArrays(1, &vao));
		glGenBuffers(1, &vbo);
		glGenBuffers(1, &ebo);

		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(MyVertex), vertices.data(), GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(MyVertex), (void*)0);

		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(MyVertex), (void*)offsetof(MyVertex, TexCoords));

		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(MyVertex), (void*)offsetof(MyVertex, Normal));

		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(MyVertex), (void*)offsetof(MyVertex, Tangent));

		glEnableVertexAttribArray(4);
		glVertexAttribIPointer(4, 4, GL_UNSIGNED_INT, sizeof(MyVertex), (void*)offsetof(MyVertex, BoneIDs));

		glEnableVertexAttribArray(5);
		glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(MyVertex), (void*)offsetof(MyVertex, Weights));

		glBindVertexArray(0);
	}

	~MyMesh()
	{
		glDeleteVertexArrays(1, &vao);
		glDeleteBuffers(1, &vbo);
		glDeleteBuffers(1, &ebo);
	}
};

class Renderer {
public:
	class ScratchBuffer {
	public:
		ScratchBuffer() : buffer_(nullptr), size_(0) {}

		~ScratchBuffer()
		{
			ozz::memory::default_allocator()->Deallocate(buffer_);
		}

		// Resizes the buffer to the new size and return the memory address.
		void* Resize(size_t _size)
		{
			if (_size > size_) {
				size_ = _size;
				ozz::memory::default_allocator()->Deallocate(buffer_);
				buffer_ = ozz::memory::default_allocator()->Allocate(_size, 16);
			}
			return buffer_;
		}

	private:
		void* buffer_;
		size_t size_;
	};
	ScratchBuffer scratch_buffer_;
	Shader myShader = Shader("ozz_model", { "a_position", "a_uv", "a_normal" });
	Shader myShader2 = Shader("ozz_model2", { "a_position", "a_uv", "a_normal", "a_tangent", "a_boneIds", "a_weights"});
	Shader nguShader = Shader("model", { "position", "textureCoords", "normal", "tangent", "jointIndices", "weights" });

	GLuint vertex_array_o_ = 0; //unused
	GLuint dynamic_array_bo_ = 0;
	GLuint dynamic_index_bo_ = 0;
	GLuint texture = 0;

public:
	bool Init()
	{
		texture = CLoadImage("../../res/testTexture.png");

		GL(GenVertexArrays(1, &vertex_array_o_));
		GL(BindVertexArray(vertex_array_o_));

		// Builds the dynamic vbo
		GL(GenBuffers(1, &dynamic_array_bo_));
		GL(GenBuffers(1, &dynamic_index_bo_));

		// Instantiate ambient textured rendering shader.
		myShader.Start();
		myShader.ConnectTextureUnits(std::vector<std::string>{"u_texture"});
		myShader.ConnectUniforms(
			std::vector<std::string>{
			"u_model"
				, "u_viewproj"
		}
		);

		myShader2.Start();
		myShader2.ConnectTextureUnits(std::vector<std::string>{"u_texture"});
		myShader2.ConnectUniforms(
			std::vector<std::string>{
			"u_model"
				, "u_view"
				, "u_proj"
		}
		);
		myShader2.ConnectUniformArray("u_bones", 128);

		nguShader.Start();
		nguShader.ConnectTextureUnits(std::vector<std::string>{"albedoT"});
		nguShader.ConnectUniforms(
			std::vector<std::string>{
			"transformationMatrix"
				, "meshTransformationMatrix"
				, "projectionMatrix"
				, "viewMatrix"
		}
		);
		nguShader.ConnectUniformArray("jointTransforms", MAX_BONES);

		return true;
	}

	bool Update(float dt)
	{
		return true;
	}

	GLuint CLoadImage(std::string path)
	{
		GLuint texture = 0;
		int width, height, channels, format;
		stbi_uc* data = stbi_load(path.c_str(), &width, &height, &channels, 0);
		if (data != nullptr)
		{
			if (channels == 1)
				format = GL_RED;
			else if (channels == 3)
				format = GL_RGB;
			else if (channels == 4)
				format = GL_RGBA;
			else
				assert(false);

			glGenTextures(1, &texture);
			glBindTexture(GL_TEXTURE_2D, texture);

			glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);

			stbi_image_free(data);

			glGenerateMipmap(GL_TEXTURE_2D);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}
		else
		{
			assert(false);
		}
		return texture;
	}

	bool DrawSkinnedMesh(const ozz::sample::Mesh& _mesh, const ozz::span<ozz::math::Float4x4> _skinning_matrices)
	{
		const int vertex_count = _mesh.vertex_count();

		// Positions and normals are interleaved to improve caching while executing
		// skinning job.
		const GLsizei positions_offset = 0;
		const GLsizei positions_stride = sizeof(float) * 3;

		const GLsizei uvs_offset = positions_offset + vertex_count * positions_stride;
		const GLsizei uvs_stride = sizeof(float) * 2;

		const GLsizei normals_offset = uvs_offset + vertex_count * uvs_stride;
		const GLsizei normals_stride = sizeof(float) * 3;

		const GLsizei tangents_offset = normals_offset + vertex_count * normals_stride;
		const GLsizei tangents_stride = sizeof(float) * 3;

		const GLsizei skinned_data_size = tangents_offset + vertex_count * tangents_stride;



		// Reallocate vertex buffer.
		const GLsizei vbo_size = skinned_data_size;
		void* vbo_map = scratch_buffer_.Resize(vbo_size);

		// Iterate mesh parts and fills vbo. Runs a skinning job per mesh part. Triangle indices are shared across parts.
		size_t processed_vertex_count = 0;
		for (size_t i = 0; i < _mesh.parts.size(); ++i)
		{
			const ozz::sample::Mesh::Part& part = _mesh.parts[i];

			// Skip this iteration if no vertex.
			const size_t part_vertex_count = part.positions.size() / 3;
			if (part_vertex_count == 0)
			{
				continue;
			}

			// Fills the job.
			ozz::geometry::SkinningJob skinning_job;
			skinning_job.vertex_count = static_cast<int>(part_vertex_count);
			const int part_influences_count = part.influences_count();

			// Clamps joints influence count according to the option.
			skinning_job.influences_count = part_influences_count;

			// Setup skinning matrices, that came from the animation stage before
			// being multiplied by inverse model-space bind-pose.
			skinning_job.joint_matrices = _skinning_matrices;

			// Setup joint's indices.
			skinning_job.joint_indices = make_span(part.joint_indices);
			skinning_job.joint_indices_stride = sizeof(uint16_t) * part_influences_count;

			// Setup joint's weights.
			if (part_influences_count > 1)
			{
				skinning_job.joint_weights = make_span(part.joint_weights);
				skinning_job.joint_weights_stride = sizeof(float) * (part_influences_count - 1);
			}

			// Setup input positions, coming from the loaded mesh.
			skinning_job.in_positions = make_span(part.positions);
			skinning_job.in_positions_stride = sizeof(float) * ozz::sample::Mesh::Part::kPositionsCpnts;

			// Setup output positions, coming from the rendering output mesh buffers.
			// We need to offset the buffer every loop.
			float* out_positions_begin = reinterpret_cast<float*>(ozz::PointerStride(vbo_map, positions_offset + processed_vertex_count * positions_stride));
			float* out_positions_end = ozz::PointerStride(out_positions_begin, part_vertex_count * positions_stride);
			skinning_job.out_positions = { out_positions_begin, out_positions_end };
			skinning_job.out_positions_stride = positions_stride;

			// Setup normals if input are provided.
			float* out_normal_begin = reinterpret_cast<float*>(ozz::PointerStride(vbo_map, normals_offset + processed_vertex_count * normals_stride));
			float* out_normal_end = ozz::PointerStride(out_normal_begin, part_vertex_count * normals_stride);
			if (part.normals.size() / ozz::sample::Mesh::Part::kNormalsCpnts == part_vertex_count)
			{
				// Setup input normals, coming from the loaded mesh.
				skinning_job.in_normals = make_span(part.normals);
				skinning_job.in_normals_stride = sizeof(float) * ozz::sample::Mesh::Part::kNormalsCpnts;

				// Setup output normals, coming from the rendering output mesh buffers.
				// We need to offset the buffer every loop.
				skinning_job.out_normals = { out_normal_begin, out_normal_end };
				skinning_job.out_normals_stride = normals_stride;
			}
			else
			{
				// Fills output with default normals.
				for (float* normal = out_normal_begin; normal < out_normal_end;
					normal = ozz::PointerStride(normal, normals_stride)) {
					normal[0] = 0.f;
					normal[1] = 1.f;
					normal[2] = 0.f;
				}
			}

			// Setup tangents if input are provided.
			float* out_tangent_begin = reinterpret_cast<float*>(ozz::PointerStride(vbo_map, tangents_offset + processed_vertex_count * tangents_stride));
			float* out_tangent_end = ozz::PointerStride(out_tangent_begin, part_vertex_count * tangents_stride);
			if (part.tangents.size() / ozz::sample::Mesh::Part::kTangentsCpnts == part_vertex_count)
			{
				// Setup input tangents, coming from the loaded mesh.
				skinning_job.in_tangents = make_span(part.tangents);
				skinning_job.in_tangents_stride = sizeof(float) * ozz::sample::Mesh::Part::kTangentsCpnts;

				// Setup output tangents, coming from the rendering output mesh buffers.
				// We need to offset the buffer every loop.
				skinning_job.out_tangents = { out_tangent_begin, out_tangent_end };
				skinning_job.out_tangents_stride = tangents_stride;
			}
			else
			{
				// Fills output with default tangents.
				for (float* tangent = out_tangent_begin; tangent < out_tangent_end;
					tangent = ozz::PointerStride(tangent, tangents_stride)) {
					tangent[0] = 1.f;
					tangent[1] = 0.f;
					tangent[2] = 0.f;
				}
			}

			// Execute the job, which should succeed unless a parameter is invalid.
			if (!skinning_job.Run())
			{
				return false;
			}

			// Copies uvs which aren't affected by skinning.
			if (part_vertex_count == part.uvs.size() / ozz::sample::Mesh::Part::kUVsCpnts)
			{
				// Optimal path used when the right number of uvs is provided.
				memcpy(ozz::PointerStride(vbo_map, uvs_offset + processed_vertex_count * uvs_stride), array_begin(part.uvs), part_vertex_count * uvs_stride);
			}
			else
			{
				// Un-optimal path used when the right number of uvs is not provided.
				assert(false);
			}

			// Some more vertices were processed.
			processed_vertex_count += part_vertex_count;
		}

		GL(BindVertexArray(vertex_array_o_));

		// Updates dynamic vertex buffer with skinned data.
		GL(BindBuffer(GL_ARRAY_BUFFER, dynamic_array_bo_));
		GL(BufferData(GL_ARRAY_BUFFER, vbo_size, nullptr, GL_STREAM_DRAW));
		GL(BufferSubData(GL_ARRAY_BUFFER, 0, vbo_size, vbo_map));

		// Binds shader with this array buffer, depending on rendering options.
		myShader.Start();

		GL(EnableVertexAttribArray(0));
		GL(VertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, positions_stride, GL_PTR_OFFSET(positions_offset)));

		GL(EnableVertexAttribArray(1));
		GL(VertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, uvs_stride, GL_PTR_OFFSET(uvs_offset)));

		GL(EnableVertexAttribArray(2));
		GL(VertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, normals_stride, GL_PTR_OFFSET(normals_offset)));

		// Binds mw uniform
		myShader.LoadUniform("u_model", glm::identity<glm::mat4>());

		// Binds mvp uniform
		myShader.LoadUniform("u_viewproj", glm::identity<glm::mat4>());

		// Binds default texture
		GL(BindTexture(GL_TEXTURE_2D, texture));

		// Maps the index dynamic buffer and update it.
		GL(BindBuffer(GL_ELEMENT_ARRAY_BUFFER, dynamic_index_bo_));
		const ozz::sample::Mesh::TriangleIndices& indices = _mesh.triangle_indices;
		GL(BufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(ozz::sample::Mesh::TriangleIndices::value_type), array_begin(indices), GL_STREAM_DRAW));

		// Draws the mesh.
		static_assert(sizeof(ozz::sample::Mesh::TriangleIndices::value_type) == 2, "Expects 2 bytes indices.");
		GL(DrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_SHORT, 0));

		// Unbinds.
		GL(BindBuffer(GL_ARRAY_BUFFER, 0));
		GL(BindTexture(GL_TEXTURE_2D, 0));
		GL(BindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
		myShader.Stop();
		glBindVertexArray(0);

		return true;
	}
	
	bool DrawMyMesh(MyMesh* _mesh)
	{
		myShader2.Start();

		GL(BindVertexArray(_mesh->vao));
		GL(BindBuffer(GL_ARRAY_BUFFER, _mesh->vbo));
		GL(BindBuffer(GL_ELEMENT_ARRAY_BUFFER, _mesh->ebo));
		GL(EnableVertexAttribArray(0));
		GL(EnableVertexAttribArray(1));
		GL(EnableVertexAttribArray(2));
		GL(EnableVertexAttribArray(3));
		GL(EnableVertexAttribArray(4));
		GL(EnableVertexAttribArray(5));

		// Binds mw uniform
		myShader2.LoadUniform("u_model", glm::identity<glm::mat4>());
		myShader2.LoadUniform("u_view", glm::identity<glm::mat4>());
		myShader2.LoadUniform("u_proj", glm::identity<glm::mat4>());

		// Binds mvp uniform
		for(int i = 0 ; i < 128 ; i++)
			myShader2.LoadUniform("u_bones[" + std::to_string(i) + "]", glm::identity<glm::mat4>());

		// Binds default texture
		GL(BindTexture(GL_TEXTURE_2D, texture));

		GL(DrawElements(GL_TRIANGLES, static_cast<unsigned int>(_mesh->indices.size()), GL_UNSIGNED_INT, 0));

		// Unbinds.
		GL(BindBuffer(GL_ARRAY_BUFFER, 0));
		GL(BindTexture(GL_TEXTURE_2D, 0));
		GL(BindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
		GL(BindVertexArray(0));

		myShader2.Stop();

		return true;
	}

	bool DrawNGU(Camera* camera, Entity* entity)
	{
		// Prepare
		glEnable(GL_MULTISAMPLE);         // turn on MSAA
		glEnable(GL_FRAMEBUFFER_SRGB);    // if using sRGB framebuffer (optional)

		glEnable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		glDisable(GL_STENCIL_TEST);
		nguShader.Start();
		nguShader.LoadUniform("viewMatrix", camera->viewMatrix);
		nguShader.LoadUniform("projectionMatrix", camera->projectionMatrix);

		// Bind Entity
		nguShader.LoadUniform("transformationMatrix", entity->transformationMatrix);
		if (entity->animator != nullptr)
		{
			const std::vector<glm::mat4>& boneMatrices = entity->animator->GetFinalBoneMatrices();
			for (int i = 0; i < MAX_BONES; i++)
				nguShader.LoadUniform(std::string(std::string("jointTransforms[") + std::to_string(i).c_str() + std::string("]")).c_str(), boneMatrices.at(i));
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
			nguShader.BindTexture(0, GL_TEXTURE_2D, "albedoT", mesh->albedoT);

			// Render Mesh Instances
			for (Transformable instance : mesh->instances)
			{
				nguShader.LoadUniform("meshTransformationMatrix", instance.transformationMatrix);
				glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(mesh->indices.size()), GL_UNSIGNED_INT, 0);
			}
			glBindVertexArray(0);;
		}
		return true;
	}
};

class OZZTester : public Listener {
private:
	
private:
	Display& display_;
	Renderer renderer_;
	ozz::sample::PlaybackController controller_;
	ozz::animation::Skeleton skeleton_;
	ozz::animation::Animation animation_;
	ozz::animation::SamplingJob::Context context_;
	ozz::vector<ozz::math::SoaTransform> locals_;
	ozz::vector<ozz::math::Float4x4> models_;
	ozz::vector<ozz::math::Float4x4> skinning_matrices_;
	ozz::vector<ozz::sample::Mesh> meshes_;
	ozz::vector<MyMesh*> my_meshes_;


	Entity* entity;
	Camera* camera;
	int renderer = 0;
	const int maxRenderer = 3;

public:
	OZZTester(Display& display) : display_(display) {}

	bool Init()
	{
		if (!ozz::sample::LoadSkeleton("../../res/skeleton.ozz", &skeleton_))
		{
			ozz::log::Err() << "LoadSkeleton error." << std::endl;
			return false;
		}

		if (!ozz::sample::LoadAnimation("../../res/animation.ozz", &animation_))
		{
			ozz::log::Err() << "LoadAnimation error." << std::endl;
			return false;
		}

		if (skeleton_.num_joints() != animation_.num_tracks())
		{
			ozz::log::Err() << "Joint count mismatch error." << std::endl;
			return false;
		}

		const int num_soa_joints = skeleton_.num_soa_joints();
		locals_.resize(num_soa_joints);
		const int num_joints = skeleton_.num_joints();
		models_.resize(num_joints);

		context_.Resize(num_joints);

		if (!ozz::sample::LoadMeshes("../../res/mesh.ozz", &meshes_))
		{
			ozz::log::Err() << "LoadMeshes error." << std::endl;
			return false;
		}

		size_t num_skinning_matrices = 0;
		for (const ozz::sample::Mesh& mesh : meshes_)
		{
			my_meshes_.push_back(new MyMesh(mesh));
			num_skinning_matrices = ozz::math::Max(num_skinning_matrices, mesh.joint_remaps.size());
		}

		skinning_matrices_.resize(num_skinning_matrices);

		for (const ozz::sample::Mesh& mesh : meshes_)
		{
			if (num_joints < mesh.highest_joint_index())
			{
				ozz::log::Err() << "Mesh joint count mismatch error." << std::endl;
				return false;
			}
		}

		if (!renderer_.Init())
			return false;

		KeyboardV2::Init(display_);
		KeyboardV2::AddListener(this);

		camera = new Camera(glm::vec3(0, 170.571, 76.5048), glm::vec3(0, 0, 0), 90.0f, display_.windowSize, 0.1f, 1000.0f);
		entity = new Entity(LoadModel("TPose.fbx"));
		entity->animator = new Animator((*entity->model->animations.begin()).second);
		
		return true;
	}

	bool Update(float _dt) 
	{
		controller_.Update(animation_, (float)(_dt / 1000.0));

		ozz::animation::SamplingJob sampling_job;
		sampling_job.animation = &animation_;
		sampling_job.context = &context_;
		sampling_job.ratio = controller_.time_ratio();
		sampling_job.output = make_span(locals_);
		if (!sampling_job.Run())
			return false;

		ozz::animation::LocalToModelJob ltm_job;
		ltm_job.skeleton = &skeleton_;
		ltm_job.input = make_span(locals_);
		ltm_job.output = make_span(models_);
		if (!ltm_job.Run())
			return false;

		return true;
	}

	bool Render(double dt)
	{
		bool success = true;

		// Early Render
		{
			GL(ClearDepth(1.f));
			GL(ClearColor(.4f, .42f, .38f, 1.f));
			GL(Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

			// Setup default states
			GL(Enable(GL_CULL_FACE));
			GL(CullFace(GL_BACK));
			GL(Enable(GL_DEPTH_TEST));
			GL(DepthMask(GL_TRUE));
			GL(DepthFunc(GL_LEQUAL));
		}

		// OZZ Render
		{
			if (renderer == 0)
			{
				for (const ozz::sample::Mesh& mesh : meshes_)
				{
					for (size_t i = 0; i < mesh.joint_remaps.size(); ++i)
					{
						skinning_matrices_[i] = models_[mesh.joint_remaps[i]] * mesh.inverse_bind_poses[i];
					}

					success &= renderer_.DrawSkinnedMesh(mesh, make_span(skinning_matrices_));
				}
			}
			else if (renderer == 1)
			{
				entity->animator->UpdateAnimation(dt / 1000.0);
				entity->CreateTransformationMatrix();

				camera->Update(dt);
				success &= renderer_.DrawNGU(camera, entity);
			}
			else if (renderer == 2)
			{

			}
		}

		// End Render
		{
			// Update Display and Get Events
			glfwSwapBuffers(display_.window);
			glfwPollEvents();
		}

		return success;
	}

	bool Loop()
	{
		static bool init = false;
		static std::chrono::high_resolution_clock::time_point lastTime = std::chrono::high_resolution_clock::now();
		if (!init) { init = true; lastTime = std::chrono::high_resolution_clock::now(); }
		static const double fps_constant = 1000.0 / 144.0;
		static const double ups_constant = 1000.0 / 24.0;

		static double fps_timer = 0.0;
		static double ups_timer = 0.0;

		std::chrono::high_resolution_clock::time_point currentTime = std::chrono::high_resolution_clock::now();
		double elapsed_ms = std::chrono::duration<double, std::milli>(currentTime - lastTime).count();
		lastTime = currentTime;

		fps_timer += elapsed_ms;
		ups_timer += elapsed_ms;

		bool success = true;
		if (fps_timer >= fps_constant)
		{
			success &= Render(fps_timer);
			fps_timer = 0.0;
		}

		if (ups_timer >= ups_constant)
		{
			success &= Update(ups_timer);
			ups_timer = 0.0;
		}

		success &= (glfwWindowShouldClose(display_.window) == 0);

		return success;
	}

	bool key_callback(int key, int scancode, int action, int mods)
	{
		if (key == GLFW_KEY_ENTER && action == GLFW_RELEASE)
		{
			renderer++;
			renderer %= maxRenderer;
		}
		std::cout << "Renderer: " << renderer << std::endl;
		return false;
	}

	bool character_callback(unsigned int codepoint)
	{
		return false;
	}

};

int main()
{
	if (Display display; display.Init())
	{
		if (OZZTester ozzTester(display); ozzTester.Init())
		{
			while (ozzTester.Loop());
		}
	}
	return 0;
}
#endif