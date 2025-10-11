#pragma once

#define GL_PTR_OFFSET(i) reinterpret_cast<void*>(static_cast<intptr_t>(i))

#include <GLFW/glfw3.h>
#include <ozz/geometry/runtime/skinning_job.h>
#include "camera.h"
static const char* VS = 
"#version 330\n"
"in vec2 a_uv;\n"
"out vec2 v_vertex_uv;\n"
"void PassUv() {\n"
"  v_vertex_uv = a_uv;\n"
"}\n"
"uniform mat4 u_model;\n mat4 GetWorldMatrix() {return u_model;}\n"
"uniform mat4 u_viewproj;\n"
"in vec3 a_position;\n"
"in vec3 a_normal;\n"
"in vec4 a_color;\n"
"out vec3 v_world_normal;\n"
"out vec4 v_vertex_color;\n"
"void main() {\n"
"  mat4 world_matrix = GetWorldMatrix();\n"
"  vec4 vertex = vec4(a_position.xyz, 1.);\n"
"  gl_Position = u_viewproj * world_matrix * vertex;\n"
"  mat3 cross_matrix = mat3(\n"
"    cross(world_matrix[1].xyz, world_matrix[2].xyz),\n"
"    cross(world_matrix[2].xyz, world_matrix[0].xyz),\n"
"    cross(world_matrix[0].xyz, world_matrix[1].xyz));\n"
"  float invdet = 1.0 / dot(cross_matrix[2], world_matrix[2].xyz);\n"
"  mat3 normal_matrix = cross_matrix * invdet;\n"
"  v_world_normal = normal_matrix * a_normal;\n"
"  v_vertex_color = a_color;\n"
"  PassUv();\n"
"}\n";



static const char* FS = "#version 330\n"
"vec4 GetAmbient(vec3 _world_normal) {\n"
"  vec3 normal = normalize(_world_normal);\n"
"  vec3 alpha = (normal + 1.) * .5;\n"
"  vec2 bt = mix(vec2(.3, .7), vec2(.4, .8), alpha.xz);\n"
"  vec3 ambient = mix(vec3(bt.x, .3, bt.x), vec3(bt.y, .8, bt.y), "
"alpha.y);\n"
"  return vec4(ambient, 1.);\n"
"}\n"
"uniform sampler2D u_texture;\n"
"in vec3 v_world_normal;\n"
"in vec4 v_vertex_color;\n"
"in vec2 v_vertex_uv;\n"
"out vec4 o_color;\n"
"void main() {\n"
"  vec4 ambient = GetAmbient(v_world_normal);\n"
"  o_color = ambient *\n"
"                 v_vertex_color *\n"
"                 texture(u_texture, v_vertex_uv);\n"
"}\n";

const uint8_t kDefaultColorsArray[][4] = {
    {255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255},
    {255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255},
    {255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255},
    {255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255},
    {255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255},
    {255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255},
    {255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255},
    {255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255},
    {255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255},
    {255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255},
    {255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255},
    {255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255},
    {255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255},
    {255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255},
    {255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255},
    {255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255},
    {255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255},
    {255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255},
    {255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255},
    {255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255},
    {255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255},
    {255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255},
    {255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255},
    {255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255}};

const float kDefaultNormalsArray[][3] = {
    {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f},
    {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f},
    {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f},
    {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f},
    {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f},
    {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f},
    {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f},
    {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f},
    {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f},
    {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f},
    {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f},
    {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f},
    {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f},
    {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f},
    {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f},
    {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}};

const float kDefaultUVsArray[][2] = {
    {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f},
    {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f},
    {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f},
    {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f},
    {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f},
    {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f},
    {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f},
    {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f},
    {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f},
    {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f},
    {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f}};

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

void glUniformMat4(ozz::math::Float4x4 _mat4, GLint _uniform) {
	float values[16];
	ozz::math::StorePtrU(_mat4.cols[0], values + 0);
	ozz::math::StorePtrU(_mat4.cols[1], values + 4);
	ozz::math::StorePtrU(_mat4.cols[2], values + 8);
	ozz::math::StorePtrU(_mat4.cols[3], values + 12);
	GL(UniformMatrix4fv(_uniform, 1, false, values));
}


namespace ozz {
	namespace sample {
        class ScratchBuffer {
        public:
            ScratchBuffer() : buffer_(nullptr), size_(0) {}
            ~ScratchBuffer() {
                memory::default_allocator()->Deallocate(buffer_);
            }

            // Resizes the buffer to the new size and return the memory address.
            void* Resize(size_t _size) {
                if (_size > size_) {
                    size_ = _size;
                    memory::default_allocator()->Deallocate(buffer_);
                    buffer_ = memory::default_allocator()->Allocate(_size, 16);
                }
                return buffer_;
            }

        private:
            void* buffer_;
            size_t size_;
        };

		class Shader {
		public:
			// Shader program
			GLuint program_;

			// Vertex and fragment shaders
			GLuint vertex_;
			GLuint fragment_;

			// Uniform locations, in the order they were requested.
			ozz::vector<GLint> uniforms_;

			// Varying locations, in the order they were requested.
			ozz::vector<GLint> attribs_;

			GLint attrib(int _index) const { return attribs_[_index]; }

			GLint uniform(int _index) const { return uniforms_[_index]; }

			bool success = true;

			Shader()
			{
				
			}

			bool Init()
			{
				success = InternalBuild(1, &VS, 1, &FS);

				success &= FindAttrib("a_uv");

				return success;
			}

			void Bind(const math::Float4x4& _model,
				const math::Float4x4& _view_proj,
				GLsizei _pos_stride, GLsizei _pos_offset,
				GLsizei _normal_stride, GLsizei _normal_offset,
				GLsizei _color_stride, GLsizei _color_offset,
				bool _color_float, GLsizei _uv_stride,
				GLsizei _uv_offset) {

				GL(UseProgram(program_));

				const GLint position_attrib = attrib(0);
				GL(EnableVertexAttribArray(position_attrib));
				GL(VertexAttribPointer(position_attrib, 3, GL_FLOAT, GL_FALSE, _pos_stride,
					GL_PTR_OFFSET(_pos_offset)));

				const GLint normal_attrib = attrib(1);
				GL(EnableVertexAttribArray(normal_attrib));
				GL(VertexAttribPointer(normal_attrib, 3, GL_FLOAT, GL_FALSE, _normal_stride,
					GL_PTR_OFFSET(_normal_offset)));

				const GLint color_attrib = attrib(2);
				GL(EnableVertexAttribArray(color_attrib));
				GL(VertexAttribPointer(
					color_attrib, 4, _color_float ? GL_FLOAT : GL_UNSIGNED_BYTE,
					!_color_float, _color_stride, GL_PTR_OFFSET(_color_offset)));

				// Binds mw uniform
				glUniformMat4(_model, uniform(0));

				// Binds mvp uniform
				glUniformMat4(_view_proj, uniform(1));

				const GLint uv_attrib = attrib(3);
				GL(EnableVertexAttribArray(uv_attrib));
				GL(VertexAttribPointer(uv_attrib, 2, GL_FLOAT, GL_FALSE, _uv_stride, GL_PTR_OFFSET(_uv_offset)));
			}

			bool InternalBuild(int _vertex_count, const char** _vertex, int _fragment_count, const char** _fragment)
			{
				bool success = true;

				success &= BuildFromSource(_vertex_count, _vertex, _fragment_count, _fragment);

				// Binds default attributes
				success &= FindAttrib("a_position");
				success &= FindAttrib("a_normal");
				success &= FindAttrib("a_color");

				// Binds default uniforms
				success &= BindUniform("u_model");
				success &= BindUniform("u_viewproj");

				return success;
			}

			bool BuildFromSource(int _vertex_count, const char** _vertex,
				int _fragment_count, const char** _fragment) {
				// Tries to compile shaders.
				GLuint vertex_shader = 0;
				if (_vertex) {
					vertex_shader = CompileShader(GL_VERTEX_SHADER, _vertex_count, _vertex);
					if (!vertex_shader) {
						return false;
					}
				}
				GLuint fragment_shader = 0;
				if (_fragment) {
					fragment_shader =
						CompileShader(GL_FRAGMENT_SHADER, _fragment_count, _fragment);
					if (!fragment_shader) {
						if (vertex_shader) {
							GL(DeleteShader(vertex_shader));
						}
						return false;
					}
				}

				// Shaders are compiled, builds program.
				program_ = glCreateProgram();
				vertex_ = vertex_shader;
				fragment_ = fragment_shader;
				GL(AttachShader(program_, vertex_shader));
				GL(AttachShader(program_, fragment_shader));
				GL(LinkProgram(program_));

				int infolog_length = 0;
				GL(GetProgramiv(program_, GL_INFO_LOG_LENGTH, &infolog_length));
				if (infolog_length > 1) {
					char* info_log = reinterpret_cast<char*>(
						memory::default_allocator()->Allocate(infolog_length, alignof(char)));
					int chars_written = 0;
					glGetProgramInfoLog(program_, infolog_length, &chars_written, info_log);
					log::Err() << info_log << std::endl;
					memory::default_allocator()->Deallocate(info_log);
				}

				return true;
			}

			bool BindUniform(const char* _semantic) {
				if (!program_) {
					return false;
				}
				GLint location = glGetUniformLocation(program_, _semantic);
				if (glGetError() != GL_NO_ERROR || location == -1) {  // _semantic not found.
					return false;
				}
				uniforms_.push_back(location);
				return true;
			}

			bool FindAttrib(const char* _semantic) {
				if (!program_) {
					return false;
				}
				GLint location = glGetAttribLocation(program_, _semantic);
				if (glGetError() != GL_NO_ERROR || location == -1) {  // _semantic not found.
					return false;
				}
				attribs_.push_back(location);
				return true;
			}

			void UnbindAttribs() {
				for (size_t i = 0; i < attribs_.size(); ++i) {
					GL(DisableVertexAttribArray(attribs_[i]));
				}
			}

			void Unbind() {
				UnbindAttribs();
				GL(UseProgram(0));
			}

			GLuint CompileShader(GLenum _type, int _count, const char** _src) {
				GLuint shader = glCreateShader(_type);
				GL(ShaderSource(shader, _count, _src, nullptr));
				GL(CompileShader(shader));

				int infolog_length = 0;
				GL(GetShaderiv(shader, GL_INFO_LOG_LENGTH, &infolog_length));
				if (infolog_length > 1) {
					char* info_log = reinterpret_cast<char*>(
						memory::default_allocator()->Allocate(infolog_length, alignof(char)));
					int chars_written = 0;
					glGetShaderInfoLog(shader, infolog_length, &chars_written, info_log);
					log::Err() << info_log << std::endl;
					memory::default_allocator()->Deallocate(info_log);
				}

				int status;
				GL(GetShaderiv(shader, GL_COMPILE_STATUS, &status));
				if (status) {
					return shader;
				}

				GL(DeleteShader(shader));
				return 0;
			}
		};

		class Renderer {
            struct Color {
                float r, g, b, a;
            };

            struct VertexPNC {
                math::Float3 pos;
                math::Float3 normal;
                Color color;
            };

            struct Model {
                Model() : vbo(0), mode(GL_POINTS), count(0) {}

                ~Model() {
                    if (vbo) {
                        GL(DeleteBuffers(1, &vbo));
                        vbo = 0;
                    }
                }

                GLuint vbo;
                GLenum mode;
                GLsizei count;
            };

		public:
            ozz::sample::internal::Camera& camera;
			Shader shader;
            ScratchBuffer scratch_buffer_;
            GLuint dynamic_array_bo_ = 0U, dynamic_index_bo_ = 0U, vertex_array_o_ = 0U, checkered_texture_ = 0U;

            // Bone and joint model objects.
            Model models_[2];

            Renderer(ozz::sample::internal::Camera& camera) : camera(camera)
			{
                
			}

            ~Renderer()
            {
#ifndef EMSCRIPTEN
                if (vertex_array_o_) {
                    GL(DeleteVertexArrays(1, &vertex_array_o_));
                    vertex_array_o_ = 0;
                }
#endif  // EMSCRIPTEN

                if (dynamic_array_bo_) {
                    GL(DeleteBuffers(1, &dynamic_array_bo_));
                    dynamic_array_bo_ = 0;
                }

                if (dynamic_index_bo_) {
                    GL(DeleteBuffers(1, &dynamic_index_bo_));
                    dynamic_index_bo_ = 0;
                }

                if (checkered_texture_) {
                    GL(DeleteTextures(1, &checkered_texture_));
                    checkered_texture_ = 0;
                }
            }

            bool InitPostureRendering() {
                const float kInter = .2f;
                {  // Prepares bone mesh.
                    const math::Float3 pos[6] = {
                        math::Float3(1.f, 0.f, 0.f),     math::Float3(kInter, .1f, .1f),
                        math::Float3(kInter, .1f, -.1f), math::Float3(kInter, -.1f, -.1f),
                        math::Float3(kInter, -.1f, .1f), math::Float3(0.f, 0.f, 0.f) };
                    const math::Float3 normals[8] = {
                        Normalize(Cross(pos[2] - pos[1], pos[2] - pos[0])),
                        Normalize(Cross(pos[1] - pos[2], pos[1] - pos[5])),
                        Normalize(Cross(pos[3] - pos[2], pos[3] - pos[0])),
                        Normalize(Cross(pos[2] - pos[3], pos[2] - pos[5])),
                        Normalize(Cross(pos[4] - pos[3], pos[4] - pos[0])),
                        Normalize(Cross(pos[3] - pos[4], pos[3] - pos[5])),
                        Normalize(Cross(pos[1] - pos[4], pos[1] - pos[0])),
                        Normalize(Cross(pos[4] - pos[1], pos[4] - pos[5])) };
                    const Color color = { 1, 1, 1, 1 };
                    const VertexPNC bones[24] = {
                        {pos[0], normals[0], color}, {pos[2], normals[0], color},
                        {pos[1], normals[0], color}, {pos[5], normals[1], color},
                        {pos[1], normals[1], color}, {pos[2], normals[1], color},
                        {pos[0], normals[2], color}, {pos[3], normals[2], color},
                        {pos[2], normals[2], color}, {pos[5], normals[3], color},
                        {pos[2], normals[3], color}, {pos[3], normals[3], color},
                        {pos[0], normals[4], color}, {pos[4], normals[4], color},
                        {pos[3], normals[4], color}, {pos[5], normals[5], color},
                        {pos[3], normals[5], color}, {pos[4], normals[5], color},
                        {pos[0], normals[6], color}, {pos[1], normals[6], color},
                        {pos[4], normals[6], color}, {pos[5], normals[7], color},
                        {pos[4], normals[7], color}, {pos[1], normals[7], color} };

                    // Builds and fills the vbo.
                    Model& bone = models_[0];
                    bone.mode = GL_TRIANGLES;
                    bone.count = OZZ_ARRAY_SIZE(bones);
                    GL(GenBuffers(1, &bone.vbo));
                    GL(BindBuffer(GL_ARRAY_BUFFER, bone.vbo));
                    GL(BufferData(GL_ARRAY_BUFFER, sizeof(bones), bones, GL_STATIC_DRAW));
                    GL(BindBuffer(GL_ARRAY_BUFFER, 0));  // Unbinds.

                    //// Init bone shader.
                    //bone.shader = BoneShader::Build();
                    //if (!bone.shader) {
                    //    return false;
                    //}
                }

                {  // Prepares joint mesh.
                    const int kNumSlices = 20;
                    const int kNumPointsPerCircle = kNumSlices + 1;
                    const int kNumPointsYZ = kNumPointsPerCircle;
                    const int kNumPointsXY = kNumPointsPerCircle + kNumPointsPerCircle / 4;
                    const int kNumPointsXZ = kNumPointsPerCircle;
                    const int kNumPoints = kNumPointsXY + kNumPointsXZ + kNumPointsYZ;
                    const float kRadius = kInter;  // Radius multiplier.
                    VertexPNC joints[kNumPoints];

                    // Fills vertices.
                    int index = 0;
                    for (int j = 0; j < kNumPointsYZ; ++j) {  // YZ plan.
                        float angle = j * math::k2Pi / kNumSlices;
                        float s = sinf(angle), c = cosf(angle);
                        VertexPNC& vertex = joints[index++];
                        vertex.pos = math::Float3(0.f, c * kRadius, s * kRadius);
                        vertex.normal = math::Float3(0.f, c, s);
                        vertex.color = { 1.f, .3f, .3f, 1.f };
                    }
                    for (int j = 0; j < kNumPointsXY; ++j) {  // XY plan.
                        float angle = j * math::k2Pi / kNumSlices;
                        float s = sinf(angle), c = cosf(angle);
                        VertexPNC& vertex = joints[index++];
                        vertex.pos = math::Float3(s * kRadius, c * kRadius, 0.f);
                        vertex.normal = math::Float3(s, c, 0.f);
                        vertex.color = { .3f, .3f, 1.f, 1.f };
                    }
                    for (int j = 0; j < kNumPointsXZ; ++j) {  // XZ plan.
                        float angle = j * math::k2Pi / kNumSlices;
                        float s = sinf(angle), c = cosf(angle);
                        VertexPNC& vertex = joints[index++];
                        vertex.pos = math::Float3(c * kRadius, 0.f, -s * kRadius);
                        vertex.normal = math::Float3(c, 0.f, -s);
                        vertex.color = { .3f, 1.f, .3f, 1.f };
                    }
                    assert(index == kNumPoints);

                    // Builds and fills the vbo.
                    Model& joint = models_[1];
                    joint.mode = GL_LINE_STRIP;
                    joint.count = OZZ_ARRAY_SIZE(joints);
                    GL(GenBuffers(1, &joint.vbo));
                    GL(BindBuffer(GL_ARRAY_BUFFER, joint.vbo));
                    GL(BufferData(GL_ARRAY_BUFFER, sizeof(joints), joints, GL_STATIC_DRAW));
                    GL(BindBuffer(GL_ARRAY_BUFFER, 0));  // Unbinds.

                    // Init joint shader.
                    //joint.shader = JointShader::Build();
                    //if (!joint.shader) {
                    //    return false;
                    //}
                }

                return true;
            }

            bool InitOpenGLExtensions() {
                bool optional_success = true;
                bool success = true;  // aka mandatory extensions
#ifdef OZZ_GL_VERSION_1_5_EXT
                OZZ_INIT_GL_EXT(glBindBuffer, PFNGLBINDBUFFERPROC, success);
                OZZ_INIT_GL_EXT(glDeleteBuffers, PFNGLDELETEBUFFERSPROC, success);
                OZZ_INIT_GL_EXT(glGenBuffers, PFNGLGENBUFFERSPROC, success);
                OZZ_INIT_GL_EXT(glIsBuffer, PFNGLISBUFFERPROC, success);
                OZZ_INIT_GL_EXT(glBufferData, PFNGLBUFFERDATAPROC, success);
                OZZ_INIT_GL_EXT(glBufferSubData, PFNGLBUFFERSUBDATAPROC, success);
                OZZ_INIT_GL_EXT(glMapBuffer, PFNGLMAPBUFFERPROC, optional_success);
                OZZ_INIT_GL_EXT(glUnmapBuffer, PFNGLUNMAPBUFFERPROC, optional_success);
                OZZ_INIT_GL_EXT(glGetBufferParameteriv, PFNGLGETBUFFERPARAMETERIVPROC,
                    success);
#endif  // OZZ_GL_VERSION_1_5_EXT

#ifdef OZZ_GL_VERSION_2_0_EXT
                OZZ_INIT_GL_EXT(glAttachShader, PFNGLATTACHSHADERPROC, success);
                OZZ_INIT_GL_EXT(glBindAttribLocation, PFNGLBINDATTRIBLOCATIONPROC, success);
                OZZ_INIT_GL_EXT(glCompileShader, PFNGLCOMPILESHADERPROC, success);
                OZZ_INIT_GL_EXT(glCreateProgram, PFNGLCREATEPROGRAMPROC, success);
                OZZ_INIT_GL_EXT(glCreateShader, PFNGLCREATESHADERPROC, success);
                OZZ_INIT_GL_EXT(glDeleteProgram, PFNGLDELETEPROGRAMPROC, success);
                OZZ_INIT_GL_EXT(glDeleteShader, PFNGLDELETESHADERPROC, success);
                OZZ_INIT_GL_EXT(glDetachShader, PFNGLDETACHSHADERPROC, success);
                OZZ_INIT_GL_EXT(glDisableVertexAttribArray, PFNGLDISABLEVERTEXATTRIBARRAYPROC,
                    success);
                OZZ_INIT_GL_EXT(glEnableVertexAttribArray, PFNGLENABLEVERTEXATTRIBARRAYPROC,
                    success);
                OZZ_INIT_GL_EXT(glGetActiveAttrib, PFNGLGETACTIVEATTRIBPROC, success);
                OZZ_INIT_GL_EXT(glGetActiveUniform, PFNGLGETACTIVEUNIFORMPROC, success);
                OZZ_INIT_GL_EXT(glGetAttachedShaders, PFNGLGETATTACHEDSHADERSPROC, success);
                OZZ_INIT_GL_EXT(glGetAttribLocation, PFNGLGETATTRIBLOCATIONPROC, success);
                OZZ_INIT_GL_EXT(glGetProgramiv, PFNGLGETPROGRAMIVPROC, success);
                OZZ_INIT_GL_EXT(glGetProgramInfoLog, PFNGLGETPROGRAMINFOLOGPROC, success);
                OZZ_INIT_GL_EXT(glGetShaderiv, PFNGLGETSHADERIVPROC, success);
                OZZ_INIT_GL_EXT(glGetShaderInfoLog, PFNGLGETSHADERINFOLOGPROC, success);
                OZZ_INIT_GL_EXT(glGetShaderSource, PFNGLGETSHADERSOURCEPROC, success);
                OZZ_INIT_GL_EXT(glGetUniformLocation, PFNGLGETUNIFORMLOCATIONPROC, success);
                OZZ_INIT_GL_EXT(glGetUniformfv, PFNGLGETUNIFORMFVPROC, success);
                OZZ_INIT_GL_EXT(glGetUniformiv, PFNGLGETUNIFORMIVPROC, success);
                OZZ_INIT_GL_EXT(glGetVertexAttribfv, PFNGLGETVERTEXATTRIBFVPROC, success);
                OZZ_INIT_GL_EXT(glGetVertexAttribiv, PFNGLGETVERTEXATTRIBIVPROC, success);
                OZZ_INIT_GL_EXT(glGetVertexAttribPointerv, PFNGLGETVERTEXATTRIBPOINTERVPROC,
                    success);
                OZZ_INIT_GL_EXT(glIsProgram, PFNGLISPROGRAMPROC, success);
                OZZ_INIT_GL_EXT(glIsShader, PFNGLISSHADERPROC, success);
                OZZ_INIT_GL_EXT(glLinkProgram, PFNGLLINKPROGRAMPROC, success);
                OZZ_INIT_GL_EXT(glShaderSource, PFNGLSHADERSOURCEPROC, success);
                OZZ_INIT_GL_EXT(glUseProgram, PFNGLUSEPROGRAMPROC, success);
                OZZ_INIT_GL_EXT(glUniform1f, PFNGLUNIFORM1FPROC, success);
                OZZ_INIT_GL_EXT(glUniform2f, PFNGLUNIFORM2FPROC, success);
                OZZ_INIT_GL_EXT(glUniform3f, PFNGLUNIFORM3FPROC, success);
                OZZ_INIT_GL_EXT(glUniform4f, PFNGLUNIFORM4FPROC, success);
                OZZ_INIT_GL_EXT(glUniform1i, PFNGLUNIFORM1IPROC, success);
                OZZ_INIT_GL_EXT(glUniform2i, PFNGLUNIFORM2IPROC, success);
                OZZ_INIT_GL_EXT(glUniform3i, PFNGLUNIFORM3IPROC, success);
                OZZ_INIT_GL_EXT(glUniform4i, PFNGLUNIFORM4IPROC, success);
                OZZ_INIT_GL_EXT(glUniform1fv, PFNGLUNIFORM1FVPROC, success);
                OZZ_INIT_GL_EXT(glUniform2fv, PFNGLUNIFORM2FVPROC, success);
                OZZ_INIT_GL_EXT(glUniform3fv, PFNGLUNIFORM3FVPROC, success);
                OZZ_INIT_GL_EXT(glUniform4fv, PFNGLUNIFORM4FVPROC, success);
                OZZ_INIT_GL_EXT(glUniformMatrix2fv, PFNGLUNIFORMMATRIX2FVPROC, success);
                OZZ_INIT_GL_EXT(glUniformMatrix3fv, PFNGLUNIFORMMATRIX3FVPROC, success);
                OZZ_INIT_GL_EXT(glUniformMatrix4fv, PFNGLUNIFORMMATRIX4FVPROC, success);
                OZZ_INIT_GL_EXT(glValidateProgram, PFNGLVALIDATEPROGRAMPROC, success);
                OZZ_INIT_GL_EXT(glVertexAttrib1f, PFNGLVERTEXATTRIB1FPROC, success);
                OZZ_INIT_GL_EXT(glVertexAttrib1fv, PFNGLVERTEXATTRIB1FVPROC, success);
                OZZ_INIT_GL_EXT(glVertexAttrib2f, PFNGLVERTEXATTRIB2FPROC, success);
                OZZ_INIT_GL_EXT(glVertexAttrib2fv, PFNGLVERTEXATTRIB2FVPROC, success);
                OZZ_INIT_GL_EXT(glVertexAttrib3f, PFNGLVERTEXATTRIB3FPROC, success);
                OZZ_INIT_GL_EXT(glVertexAttrib3fv, PFNGLVERTEXATTRIB3FVPROC, success);
                OZZ_INIT_GL_EXT(glVertexAttrib4f, PFNGLVERTEXATTRIB4FPROC, success);
                OZZ_INIT_GL_EXT(glVertexAttrib4fv, PFNGLVERTEXATTRIB4FVPROC, success);
                OZZ_INIT_GL_EXT(glVertexAttribPointer, PFNGLVERTEXATTRIBPOINTERPROC, success);
#endif  // OZZ_GL_VERSION_2_0_EXT

#ifdef OZZ_GL_VERSION_3_0_EXT
                OZZ_INIT_GL_EXT(glBindVertexArray, PFNGLBINDVERTEXARRAYPROC, success);
                OZZ_INIT_GL_EXT(glDeleteVertexArrays, PFNGLDELETEVERTEXARRAYSPROC, success);
                OZZ_INIT_GL_EXT(glGenVertexArrays, PFNGLGENVERTEXARRAYSPROC, success);
                OZZ_INIT_GL_EXT(glIsVertexArray, PFNGLISVERTEXARRAYPROC, success);
#endif  // OZZ_GL_VERSION_3_0_EXT

                if (!success) {
                    log::Err() << "Failed to initialize all mandatory GL extensions."
                        << std::endl;
                    return false;
                }
                if (!optional_success) {
                    log::Log() << "Failed to initialize some optional GL extensions."
                        << std::endl;
                }

                //GL_ARB_instanced_arrays_supported =
                //    glfwExtensionSupported("GL_ARB_instanced_arrays") != 0;
                //if (GL_ARB_instanced_arrays_supported) {
                //    log::Log() << "Optional GL_ARB_instanced_arrays extensions found."
                //        << std::endl;
                //    success = true;
                //    OZZ_INIT_GL_EXT_N(glVertexAttribDivisor_, "glVertexAttribDivisorARB",
                //        PFNGLVERTEXATTRIBDIVISORARBPROC, success);
                //    OZZ_INIT_GL_EXT_N(glDrawArraysInstanced_, "glDrawArraysInstancedARB",
                //        PFNGLDRAWARRAYSINSTANCEDARBPROC, success);
                //    OZZ_INIT_GL_EXT_N(glDrawElementsInstanced_, "glDrawElementsInstancedARB",
                //        PFNGLDRAWELEMENTSINSTANCEDARBPROC, success);
                //    if (!success) {
                //        log::Err()
                //            << "Failed to setup GL_ARB_instanced_arrays, feature is disabled."
                //            << std::endl;
                //        GL_ARB_instanced_arrays_supported = false;
                //    }
                //}
                //else {
                //    log::Log() << "Optional GL_ARB_instanced_arrays extensions not found."
                //        << std::endl;
                //}
                return true;
            }

            bool InitCheckeredTexture() {
                const int kWidth = 1024;
                const int kCases = 64;

                GL(GenTextures(1, &checkered_texture_));
                GL(BindTexture(GL_TEXTURE_2D, checkered_texture_));
                GL(TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
                GL(TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));
                GL(TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
                GL(TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    GL_LINEAR_MIPMAP_LINEAR));
                GL(PixelStorei(GL_UNPACK_ALIGNMENT, 1));

                // Allocates for biggest mip level.
                const size_t buffer_size = 3 * kWidth * kWidth;
                uint8_t* pixels = reinterpret_cast<uint8_t*>(
                    memory::default_allocator()->Allocate(buffer_size, 16));

                // Create the checkered pattern on all mip levels.
                int level_width = kWidth;
                for (int level = 0; level_width > 0; ++level, level_width /= 2) {
                    if (level_width >= kCases) {
                        const int case_width = level_width / kCases;
                        for (int j = 0; j < level_width; ++j) {
                            const int cpntj = (j / case_width) & 1;
                            for (int i = 0; i < kCases; ++i) {
                                const int cpnti = i & 1;
                                const bool white_case = (cpnti ^ cpntj) != 0;
                                const uint8_t cpntr =
                                    white_case ? 0xff : j * 255 / level_width & 0xff;
                                const uint8_t cpntg = white_case ? 0xff : i * 255 / kCases & 0xff;
                                const uint8_t cpntb = white_case ? 0xff : 0;

                                const int case_start = j * level_width + i * case_width;
                                for (int k = case_start; k < case_start + case_width; ++k) {
                                    pixels[k * 3 + 0] = cpntr;
                                    pixels[k * 3 + 1] = cpntg;
                                    pixels[k * 3 + 2] = cpntb;
                                }
                            }
                        }
                    }
                    else {
                        // Mimaps where width is smaller than the number of cases.
                        for (int j = 0; j < level_width; ++j) {
                            for (int i = 0; i < level_width; ++i) {
                                pixels[(j * level_width + i) * 3 + 0] = 0x7f;
                                pixels[(j * level_width + i) * 3 + 1] = 0x7f;
                                pixels[(j * level_width + i) * 3 + 2] = 0x7f;
                            }
                        }
                    }

                    GL(TexImage2D(GL_TEXTURE_2D, level, GL_RGB, level_width, level_width, 0,
                        GL_RGB, GL_UNSIGNED_BYTE, pixels));
                }

                GL(BindTexture(GL_TEXTURE_2D, 0));
                memory::default_allocator()->Deallocate(pixels);

                return true;
            }

			bool Init()
			{
                if (!InitOpenGLExtensions()) {
                    return false;
                }

                if (!InitPostureRendering()) {
                    return false;
                }
                if (!InitCheckeredTexture()) {
                    return false;
                }

                // Build and bind vertex array once for all
#ifndef EMSCRIPTEN
                GL(GenVertexArrays(1, &vertex_array_o_));
                GL(BindVertexArray(vertex_array_o_));
#endif  // EMSCRIPTEN

                // Builds the dynamic vbo
                GL(GenBuffers(1, &dynamic_array_bo_));
                GL(GenBuffers(1, &dynamic_index_bo_));

                return shader.Init();
			}

			int DrawSkinnedMesh(const Mesh& _mesh, const span<math::Float4x4> _skinning_matrices, const ozz::math::Float4x4& _transform)
			{
                const int vertex_count = _mesh.vertex_count();

                // Positions and normals are interleaved to improve caching while executing
                // skinning job.
                const GLsizei positions_offset = 0;
                const GLsizei positions_stride = sizeof(float) * 3;
                const GLsizei normals_offset = vertex_count * positions_stride;
                const GLsizei normals_stride = sizeof(float) * 3;
                const GLsizei tangents_offset =
                    normals_offset + vertex_count * normals_stride;
                const GLsizei tangents_stride = sizeof(float) * 3;
                const GLsizei skinned_data_size =
                    tangents_offset + vertex_count * tangents_stride;

                // Colors and uvs are contiguous. They aren't transformed, so they can be
                // directly copied from source mesh which is non-interleaved as-well.
                // Colors will be filled with white if _options.colors is false.
                // UVs will be skipped if _options.textured is false.
                const GLsizei colors_offset = skinned_data_size;
                const GLsizei colors_stride = sizeof(uint8_t) * 4;
                const GLsizei colors_size = vertex_count * colors_stride;
                const GLsizei uvs_offset = colors_offset + colors_size;
                const GLsizei uvs_stride = sizeof(float) * 2;
                const GLsizei uvs_size = vertex_count * uvs_stride;
                const GLsizei fixed_data_size = colors_size + uvs_size;

                // Reallocate vertex buffer.
                const GLsizei vbo_size = skinned_data_size + fixed_data_size;
                void* vbo_map = scratch_buffer_.Resize(vbo_size);

                // Iterate mesh parts and fills vbo.
                // Runs a skinning job per mesh part. Triangle indices are shared
                // across parts.
                size_t processed_vertex_count = 0;
                for (size_t i = 0; i < _mesh.parts.size(); ++i) {
                    const ozz::sample::Mesh::Part& part = _mesh.parts[i];

                    // Skip this iteration if no vertex.
                    const size_t part_vertex_count = part.positions.size() / 3;
                    if (part_vertex_count == 0) {
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
                    skinning_job.joint_indices_stride =
                        sizeof(uint16_t) * part_influences_count;

                    // Setup joint's weights.
                    if (part_influences_count > 1) {
                        skinning_job.joint_weights = make_span(part.joint_weights);
                        skinning_job.joint_weights_stride =
                            sizeof(float) * (part_influences_count - 1);
                    }

                    // Setup input positions, coming from the loaded mesh.
                    skinning_job.in_positions = make_span(part.positions);
                    skinning_job.in_positions_stride =
                        sizeof(float) * ozz::sample::Mesh::Part::kPositionsCpnts;

                    // Setup output positions, coming from the rendering output mesh buffers.
                    // We need to offset the buffer every loop.
                    float* out_positions_begin = reinterpret_cast<float*>(ozz::PointerStride(
                        vbo_map, positions_offset + processed_vertex_count * positions_stride));
                    float* out_positions_end = ozz::PointerStride(
                        out_positions_begin, part_vertex_count * positions_stride);
                    skinning_job.out_positions = { out_positions_begin, out_positions_end };
                    skinning_job.out_positions_stride = positions_stride;

                    // Setup normals if input are provided.
                    float* out_normal_begin = reinterpret_cast<float*>(ozz::PointerStride(
                        vbo_map, normals_offset + processed_vertex_count * normals_stride));
                    float* out_normal_end = ozz::PointerStride(
                        out_normal_begin, part_vertex_count * normals_stride);

                    if (part.normals.size() / ozz::sample::Mesh::Part::kNormalsCpnts ==
                        part_vertex_count) {
                        // Setup input normals, coming from the loaded mesh.
                        skinning_job.in_normals = make_span(part.normals);
                        skinning_job.in_normals_stride =
                            sizeof(float) * ozz::sample::Mesh::Part::kNormalsCpnts;

                        // Setup output normals, coming from the rendering output mesh buffers.
                        // We need to offset the buffer every loop.
                        skinning_job.out_normals = { out_normal_begin, out_normal_end };
                        skinning_job.out_normals_stride = normals_stride;
                    }
                    else {
                        // Fills output with default normals.
                        for (float* normal = out_normal_begin; normal < out_normal_end;
                            normal = ozz::PointerStride(normal, normals_stride)) {
                            normal[0] = 0.f;
                            normal[1] = 1.f;
                            normal[2] = 0.f;
                        }
                    }

                    // Setup tangents if input are provided.
                    float* out_tangent_begin = reinterpret_cast<float*>(ozz::PointerStride(
                        vbo_map, tangents_offset + processed_vertex_count * tangents_stride));
                    float* out_tangent_end = ozz::PointerStride(
                        out_tangent_begin, part_vertex_count * tangents_stride);

                    if (part.tangents.size() / ozz::sample::Mesh::Part::kTangentsCpnts ==
                        part_vertex_count) {
                        // Setup input tangents, coming from the loaded mesh.
                        skinning_job.in_tangents = make_span(part.tangents);
                        skinning_job.in_tangents_stride =
                            sizeof(float) * ozz::sample::Mesh::Part::kTangentsCpnts;

                        // Setup output tangents, coming from the rendering output mesh buffers.
                        // We need to offset the buffer every loop.
                        skinning_job.out_tangents = { out_tangent_begin, out_tangent_end };
                        skinning_job.out_tangents_stride = tangents_stride;
                    }
                    else {
                        // Fills output with default tangents.
                        for (float* tangent = out_tangent_begin; tangent < out_tangent_end;
                            tangent = ozz::PointerStride(tangent, tangents_stride)) {
                            tangent[0] = 1.f;
                            tangent[1] = 0.f;
                            tangent[2] = 0.f;
                        }
                    }

                    // Execute the job, which should succeed unless a parameter is invalid.
                    if (!skinning_job.Run()) {
                        return false;
                    }

                    // Handles colors which aren't affected by skinning.
                    // Un-optimal path used when the right number of colors is not provided.
                    static_assert(sizeof(kDefaultColorsArray[0]) == colors_stride,
                        "Stride mismatch");

                    for (size_t j = 0; j < part_vertex_count;
                        j += OZZ_ARRAY_SIZE(kDefaultColorsArray)) {
                        const size_t this_loop_count = math::Min(
                            OZZ_ARRAY_SIZE(kDefaultColorsArray), part_vertex_count - j);
                        memcpy(ozz::PointerStride(
                            vbo_map, colors_offset +
                            (processed_vertex_count + j) * colors_stride),
                            kDefaultColorsArray, colors_stride * this_loop_count);
                    }

                    // Copies uvs which aren't affected by skinning.
                    if (part_vertex_count ==
                        part.uvs.size() / ozz::sample::Mesh::Part::kUVsCpnts) {
                        // Optimal path used when the right number of uvs is provided.
                        memcpy(ozz::PointerStride(
                            vbo_map, uvs_offset + processed_vertex_count * uvs_stride),
                            array_begin(part.uvs), part_vertex_count * uvs_stride);
                    }
                    else {
                        // Un-optimal path used when the right number of uvs is not provided.
                        assert(sizeof(kDefaultUVsArray[0]) == uvs_stride);
                        for (size_t j = 0; j < part_vertex_count;
                            j += OZZ_ARRAY_SIZE(kDefaultUVsArray)) {
                            const size_t this_loop_count = math::Min(
                                OZZ_ARRAY_SIZE(kDefaultUVsArray), part_vertex_count - j);
                            memcpy(ozz::PointerStride(
                                vbo_map,
                                uvs_offset + (processed_vertex_count + j) * uvs_stride),
                                kDefaultUVsArray, uvs_stride * this_loop_count);
                        }
                    }

                    // Some more vertices were processed.
                    processed_vertex_count += part_vertex_count;
                }

                if (/*_options.triangles*/true) {
                    // Updates dynamic vertex buffer with skinned data.
                    GL(BindBuffer(GL_ARRAY_BUFFER, dynamic_array_bo_));
                    GL(BufferData(GL_ARRAY_BUFFER, vbo_size, nullptr, GL_STREAM_DRAW));
                    GL(BufferSubData(GL_ARRAY_BUFFER, 0, vbo_size, vbo_map));

                    // Binds shader with this array buffer, depending on rendering options.
                    shader.Bind(
                        _transform, camera.view_proj(), positions_stride, positions_offset,
                        normals_stride, normals_offset, colors_stride, colors_offset, false,
                        uvs_stride, uvs_offset);

                    // Binds default texture
                    GL(BindTexture(GL_TEXTURE_2D, checkered_texture_));

                    // Maps the index dynamic buffer and update it.
                    GL(BindBuffer(GL_ELEMENT_ARRAY_BUFFER, dynamic_index_bo_));
                    const Mesh::TriangleIndices& indices = _mesh.triangle_indices;
                    GL(BufferData(GL_ELEMENT_ARRAY_BUFFER,
                        indices.size() * sizeof(Mesh::TriangleIndices::value_type),
                        array_begin(indices), GL_STREAM_DRAW));

                    // Draws the mesh.
                    static_assert(sizeof(Mesh::TriangleIndices::value_type) == 2,
                        "Expects 2 bytes indices.");
                    GL(DrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()),
                        GL_UNSIGNED_SHORT, 0));

                    // Unbinds.
                    GL(BindBuffer(GL_ARRAY_BUFFER, 0));
                    GL(BindTexture(GL_TEXTURE_2D, 0));
                    GL(BindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
                    shader.Unbind();
                }

                return 0;
            }

		};
	}
}