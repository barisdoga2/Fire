#ifndef MAIN

#define STB_IMAGE_IMPLEMENTATION

#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <vector>
#include <unordered_map>
#include <map>

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <glm/glm.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#define GL_DEBUG
#ifdef GL_DEBUG
#define GL(_f)                                                     \
  do {                                                             \
    gl##_f;                                                        \
    GLenum gl_err = glGetError();                                  \
    if (gl_err != 0) {                                             \
      std::cout << "GL error 0x" << std::hex << gl_err			   \
                      << " returned from 'gl" << #_f << std::endl; \
    }                                                              \
    assert(gl_err == GL_NO_ERROR);                                 \
  } while (void(0), 0)
#else
#define GL(_f) gl##_f
#endif

static inline glm::mat4 AiToGlm(const aiMatrix4x4& m)
{
	glm::mat4 result;
	result[0][0] = m.a1; result[1][0] = m.a2; result[2][0] = m.a3; result[3][0] = m.a4;
	result[0][1] = m.b1; result[1][1] = m.b2; result[2][1] = m.b3; result[3][1] = m.b4;
	result[0][2] = m.c1; result[1][2] = m.c2; result[2][2] = m.c3; result[3][2] = m.c4;
	result[0][3] = m.d1; result[1][3] = m.d2; result[2][3] = m.d3; result[3][3] = m.d4;
	return result;
}

static inline glm::vec3 AiToGlm(const aiVector3D& m)
{
	glm::vec3 result;
	result.x = m.x;
	result.y = m.y;
	result.z = m.z;
	return result;
}

static inline glm::quat AiToGlm(const aiQuaternion& q)
{
	return glm::quat(q.w, q.x, q.y, q.z);
}

static inline glm::mat4 CreateTransformMatrix(const glm::vec3& position, const glm::vec3& rotationDeg, const glm::vec3& scale)
{
	// Convert degrees to radians
	glm::vec3 rotation = glm::radians(rotationDeg);

	// Build individual matrices
	glm::mat4 translation = glm::translate(glm::mat4(1.0f), position);
	glm::mat4 rotationMat = glm::yawPitchRoll(rotation.y, rotation.x, rotation.z);
	glm::mat4 scaling = glm::scale(glm::mat4(1.0f), scale);

	// Combine (T * R * S)
	glm::mat4 transform = translation * rotationMat * scaling;

	return transform;
}

static const char* VS = "\n"
"\n"
"#version 400 core\n"
"\n"
"layout (location = 0) in vec3 position;\n"
"layout (location = 1) in vec2 uv;\n"
"layout (location = 2) in vec3 normal;\n"
"layout (location = 3) in vec3 tangent;\n"
"layout (location = 4) in vec3 bitangent;\n"
"layout (location = 5) in ivec4 boneIds;\n"
"layout (location = 6) in vec4 weights;\n"
"uniform mat4 model;\n"
"uniform mat4 view;\n"
"uniform mat4 proj;\n"
"uniform mat4 boneMatrices[200];\n"
"uniform int animated;\n"
"out vec2 pass_uv;\n"
"\n"
"void main()\n"
"{\n"
"   pass_uv = uv + vec2(normal.x, tangent.y + bitangent.z) * 0.000025;\n"
"	vec4 totalPosition = vec4(0.0f);\n"
"	if(animated == 1)\n"
"	{\n"
"		for (int i = 0; i < 4; i++)\n"
"		{\n"
"			if (boneIds[i] == -1)\n"
"				continue; \n"
"			if (boneIds[i] >= 200)\n"
"			{\n"
"				totalPosition = vec4(position, 1.0f);\n"
"				break; \n"
"			}\n"
"			vec4 localPosition = boneMatrices[boneIds[i]] * vec4(position, 1.0f); \n"
"			totalPosition += localPosition * weights[i]; \n"
"		}\n"
"	}\n"
"	else\n"
"	{\n"
"		totalPosition = vec4(position, 1.0);\n"
"	}\n"
"	gl_Position = proj * view * model * totalPosition;\n"
"}\n"
"\n";

static const char* FS = "\n"
"\n"
"#version 400 core\n"
"\n"
"in vec2 pass_uv;\n"
"uniform sampler2D diffuse;\n"
"layout (location = 0) out vec4 out_color;\n"
"\n"
"void main()\n"
"{\n"
"	out_color = vec4(texture(diffuse, pass_uv).rgb, 1.0);\n"
"}\n"
"\n";

std::unordered_map<std::string, float> upperBodyMask = {
{"mixamorig:Hips", 0.0f},
{"mixamorig:Spine", 0.3f},
{"mixamorig:Spine1", 0.6f},
{"mixamorig:Spine2", 1.0f},

{"mixamorig:Bowl5", 1.0f},
{"mixamorig:Bowl1", 1.0f},
{"mixamorig:Bowl6", 1.0f},
{"mixamorig:Bowl2", 1.0f},
{"mixamorig:Bowl7", 1.0f},
{"mixamorig:Bowl3", 1.0f},
{"mixamorig:Bowl8", 1.0f},
{"mixamorig:Bowl4", 1.0f},

{"mixamorig:Neck", 1.0f},
{"mixamorig:Head", 1.0f},
{"mixamorig:Leye", 1.0f},
{"mixamorig:Reye", 1.0f},
{"mixamorig:Head", 1.0f},
{"mixamorig:HeadTop_End", 1.0f},
{"mixamorig:LeftShoulder", 1.0f},
{"mixamorig:LeftArm", 1.0f},
{"mixamorig:LeftForeArm", 1.0f},
{"mixamorig:LeftHand", 1.0f},
{"mixamorig:RightShoulder", 1.0f},
{"mixamorig:RightArm", 1.0f},
{"mixamorig:RightForeArm", 1.0f},
{"mixamorig:RightHand", 1.0f},

{"mixamorig:RightHandPinky1", 1.0f},
{"mixamorig:RightHandPinky2", 1.0f},
{"mixamorig:RightHandPinky3", 1.0f},
{"mixamorig:RightHandPinky4", 1.0f},
{"mixamorig:RightHandRing1", 1.0f},
{"mixamorig:RightHandRing2", 1.0f},
{"mixamorig:RightHandRing3", 1.0f},
{"mixamorig:RightHandRing4", 1.0f},
{"mixamorig:RightHandMiddle1", 1.0f},
{"mixamorig:RightHandMiddle2", 1.0f},
{"mixamorig:RightHandMiddle3", 1.0f},
{"mixamorig:RightHandMiddle4", 1.0f},
{"mixamorig:RightHandIndex1", 1.0f},
{"mixamorig:RightHandIndex2", 1.0f},
{"mixamorig:RightHandIndex3", 1.0f},
{"mixamorig:RightHandIndex4", 1.0f},
{"mixamorig:RightHandThumb1", 1.0f},
{"mixamorig:RightHandThumb2", 1.0f},
{"mixamorig:RightHandThumb3", 1.0f},
{"mixamorig:RightHandThumb4", 1.0f},
{"mixamorig:LeftHandPinky1", 1.0f},
{"mixamorig:LeftHandPinky2", 1.0f},
{"mixamorig:LeftHandPinky3", 1.0f},
{"mixamorig:LeftHandPinky4", 1.0f},
{"mixamorig:LeftHandRing1", 1.0f},
{"mixamorig:LeftHandRing2", 1.0f},
{"mixamorig:LeftHandRing3", 1.0f},
{"mixamorig:LeftHandRing4", 1.0f},
{"mixamorig:LeftHandMiddle1", 1.0f},
{"mixamorig:LeftHandMiddle2", 1.0f},
{"mixamorig:LeftHandMiddle3", 1.0f},
{"mixamorig:LeftHandMiddle4", 1.0f},
{"mixamorig:LeftHandIndex1", 1.0f},
{"mixamorig:LeftHandIndex2", 1.0f},
{"mixamorig:LeftHandIndex3", 1.0f},
{"mixamorig:LeftHandIndex4", 1.0f},
{"mixamorig:LeftHandThumb1", 1.0f},
{"mixamorig:LeftHandThumb2", 1.0f},
{"mixamorig:LeftHandThumb3", 1.0f},
{"mixamorig:LeftHandThumb4", 1.0f}
};

std::unordered_map<std::string, float> lowerBodyMask = {
	{"mixamorig:Hips", 1.0f},
	{"mixamorig:LeftUpLeg", 1.0f},
	{"mixamorig:LeftLeg", 1.0f},
	{"mixamorig:LeftFoot", 1.0f},
	{"mixamorig:RightUpLeg", 1.0f},
	{"mixamorig:RightLeg", 1.0f},
	{"mixamorig:RightFoot", 1.0f},
	{"mixamorig:RightToeBase", 1.0f},
	{"mixamorig:RightToe_End", 1.0f},
	{"mixamorig:LeftToeBase", 1.0f},
	{"mixamorig:LeftToe_End", 1.0f},
};

struct AssimpNodeData
{
	glm::mat4x4 transformation;
	std::string name;
	int childrenCount;
	std::vector<AssimpNodeData> children;
};

struct EasyVertex {
	glm::vec3 position = glm::vec3(0, 0, 0);
	glm::vec2 uv = glm::vec2(0, 0);
	glm::vec3 normal = glm::vec3(0, 0, 0);
	glm::vec3 tangent = glm::vec3(0, 0, 0);
	glm::vec3 bitangent = glm::vec3(0, 0, 0);
	glm::ivec4 boneIds = glm::ivec4(-1, -1, -1, -1);
	glm::vec4 weights = glm::vec4(0, 0, 0, 0);
};

struct EasyBoneInfo
{
	int id;
	glm::mat4 offset;
};

struct KeyPosition
{
	glm::vec3 position;
	double timeStamp;
};

struct KeyRotation
{
	glm::quat orientation;
	double timeStamp;
};

struct KeyScale
{
	glm::vec3 scale;
	double timeStamp;
};

class EasyTexture {
public:
	static GLuint Load(std::string path)
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

	static GLuint Load(const aiTexture* embedded)
	{
		GLuint texture = 0;

		int width, height, channels;
		unsigned char* data;

		if (embedded->mHeight == 0)
		{
			data = stbi_load_from_memory(reinterpret_cast<unsigned char*>(embedded->pcData), embedded->mWidth, &width, &height, &channels, 0);
		}
		else
		{
			width = embedded->mWidth;
			height = embedded->mHeight;
			channels = 4;
			data = reinterpret_cast<unsigned char*>(embedded->pcData);
		}

		if (data)
		{
			glGenTextures(1, &texture);
			glBindTexture(GL_TEXTURE_2D, texture);
			GLenum format = (channels == 4 ? GL_RGBA : GL_RGB);
			glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
			glGenerateMipmap(GL_TEXTURE_2D);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glBindTexture(GL_TEXTURE_2D, 0);

		}

		if (embedded->mHeight != 0)
			stbi_image_free(data);

		return texture;
	}
};

class EasyShader {
public:
	GLuint prog;
	std::unordered_map<std::string, GLint> attribs;
	std::unordered_map<std::string, GLint> uniforms;

	EasyShader(const char* VS, const char* FS)
	{
		prog = glCreateProgram();

		AttachShader(GL_VERTEX_SHADER, VS);

		AttachShader(GL_FRAGMENT_SHADER, FS);

		glLinkProgram(prog);
		GLint status = GL_FALSE, log[1 << 11] = { 0 };
		GL(GetProgramiv(prog, GL_LINK_STATUS, &status));
		if (status != GL_TRUE)
		{
			glGetProgramInfoLog(prog, sizeof(log), nullptr, (GLchar*)log);
			std::cout << (std::string((GLchar*)log).length() == 0 ? "Error Loading Shader!" : (GLchar*)log) << std::endl;
		}
		glUseProgram(prog);
	}

	void Start()
	{
		GL(UseProgram(prog));
	}

	void Stop()
	{
		GL(UseProgram(0));
	}

	void BindAttribs(const std::vector<std::string>& attribs)
	{
		for (const std::string& attrib : attribs)
		{
			GLint location = glGetAttribLocation(prog, attrib.c_str());
			if (glGetError() != GL_NO_ERROR || location == -1)
			{
				std::cout << "Shader attrib not found! " << attrib.c_str() << std::endl;
			}
			else
			{
				this->attribs.insert({ attrib, location });
			}
		}
	}

	void BindUniforms(const std::vector<std::string>& uniforms)
	{
		for (const std::string& uniform : uniforms)
		{
			GLint location = glGetUniformLocation(prog, uniform.c_str());
			if (glGetError() != GL_NO_ERROR || location == -1)
			{
				std::cout << "Shader uniform not found! " << uniform.c_str() << std::endl;
			}
			else
			{
				this->uniforms.insert({ uniform, location });
			}
		}
	}

	void BindUniformArray(std::string name, int size)
	{
		for (int i = 0; i < size; i++)
		{
			GLint location = glGetUniformLocation(prog, std::string(name + "[" + std::to_string(i) + "]").c_str());
			if (glGetError() != GL_NO_ERROR || location == -1)
			{
				std::cout << "Shader uniform array not found! " << std::string(name + "[" + std::to_string(i) + "]") << std::endl;
			}
			else
			{
				this->uniforms.insert({ std::string(name + "[" + std::to_string(i) + "]"), location });
			}
		}
	}

	void LoadUniform(const std::string& uniform, const glm::vec3& value)
	{
		auto res = uniforms.find(uniform);
		if (res != uniforms.end())
		{
			GL(Uniform3f(res->second, value.x, value.y, value.z));
		}
		else
		{
			std::cout << "Shader uniform to load not found! " << uniform << std::endl;
		}
	}

	void LoadUniform(const std::string& uniform, const GLint& value)
	{
		auto res = uniforms.find(uniform);
		if (res != uniforms.end())
		{
			GL(Uniform1i(res->second, value));
		}
		else
		{
			std::cout << "Shader uniform to load not found! " << uniform << std::endl;
		}
	}

	void LoadUniform(const std::string& uniform, const glm::mat4x4& mat4x4)
	{
		auto res = uniforms.find(uniform);
		if (res != uniforms.end())
		{
			GL(UniformMatrix4fv(res->second, 1, GL_FALSE, (GLfloat*)&mat4x4[0][0]));
		}
		else
		{
			std::cout << "Shader uniform to load not found! " << uniform << std::endl;
		}

	}

	void LoadUniform(const std::string& uniform, const std::vector<glm::mat4x4>& mat4x4)
	{
		auto res = uniforms.find(uniform + "[0]");
		if (res != uniforms.end())
		{
			GL(UniformMatrix4fv(res->second, (GLsizei)mat4x4.size(), GL_FALSE, (GLfloat*)mat4x4.data()));
		}
		else
		{
			std::cout << "Shader uniform array to load not found! " << uniform << std::endl;
		}

	}

private:
	void AttachShader(GLenum type, const char* src, const GLint* length = nullptr)
	{
		GLuint shader = glCreateShader(type);
		GL(ShaderSource(shader, 1, &src, length));
		GL(CompileShader(shader));

		GLint status = GL_FALSE, log[1 << 11] = { 0 };
		GL(GetShaderiv(shader, GL_COMPILE_STATUS, &status));
		if (status != GL_TRUE)
		{
			GL(GetShaderInfoLog(shader, sizeof(log), NULL, (GLchar*)log));
			std::cout << (std::string((GLchar*)log).length() == 0 ? "Error Loading Shader!" : (GLchar*)log) << std::endl;
		}

		GL(AttachShader(prog, shader));
		GL(DeleteShader(shader));
	}

};

class EasyDisplay {
public:
	glm::tvec2<int> windowSize = glm::tvec2<int>(1920, 1080);
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

class EasyBone
{
private:
	std::vector<KeyPosition> m_Positions;
	std::vector<KeyRotation> m_Rotations;
	std::vector<KeyScale> m_Scales;
	int m_NumPositions;
	int m_NumRotations;
	int m_NumScalings;

	glm::mat4 m_LocalTransform;
	std::string m_Name;
	int m_ID;

public:

	/*reads keyframes from aiNodeAnim*/
	EasyBone(const std::string& name, int ID, const aiNodeAnim* channel) : m_Name(name), m_ID(ID), m_LocalTransform(1.0f)
	{
		m_NumPositions = channel->mNumPositionKeys;

		for (int positionIndex = 0; positionIndex < m_NumPositions; ++positionIndex)
		{
			aiVector3D aiPosition = channel->mPositionKeys[positionIndex].mValue;
			double timeStamp = channel->mPositionKeys[positionIndex].mTime;
			KeyPosition data;
			data.position = AiToGlm(aiPosition);
			data.timeStamp = timeStamp;
			m_Positions.push_back(data);
		}

		m_NumRotations = channel->mNumRotationKeys;
		for (int rotationIndex = 0; rotationIndex < m_NumRotations; ++rotationIndex)
		{
			aiQuaternion aiOrientation = channel->mRotationKeys[rotationIndex].mValue;
			double timeStamp = channel->mRotationKeys[rotationIndex].mTime;
			KeyRotation data;
			data.orientation = AiToGlm(aiOrientation);
			data.timeStamp = timeStamp;
			m_Rotations.push_back(data);
		}

		m_NumScalings = channel->mNumScalingKeys;
		for (int keyIndex = 0; keyIndex < m_NumScalings; ++keyIndex)
		{
			aiVector3D scale = channel->mScalingKeys[keyIndex].mValue;
			double timeStamp = channel->mScalingKeys[keyIndex].mTime;
			KeyScale data;
			data.scale = AiToGlm(scale);
			data.timeStamp = timeStamp;
			m_Scales.push_back(data);
		}
	}

	/*interpolates  b/w positions,rotations & scaling keys based on the curren time of
	the animation and prepares the local transformation matrix by combining all keys
	tranformations*/
	void Update(double animationTime)
	{
		const glm::mat4 translation = InterpolatePosition(animationTime);
		const glm::mat4 rotation = InterpolateRotation(animationTime);
		const glm::mat4 scale = InterpolateScaling(animationTime);
		m_LocalTransform = translation * rotation * scale;
	}


	glm::mat4 GetLocalTransform() const { return m_LocalTransform; }
	std::string GetBoneName() const { return m_Name; }
	int GetBoneID() const { return m_ID; }


	/* Gets the current index on mKeyPositions to interpolate to based on
	the current animation time*/
	int GetPositionIndex(double animationTime) const
	{
		for (int index = 0; index < m_NumPositions - 1; ++index)
		{
			if (animationTime < m_Positions[index + 1].timeStamp)
				return index;
		}
		assert(0);
		return 0;
	}

	/* Gets the current index on mKeyRotations to interpolate to based on the
	current animation time*/
	int GetRotationIndex(double animationTime) const
	{
		for (int index = 0; index < m_NumRotations - 1; ++index)
		{
			if (animationTime < m_Rotations[index + 1].timeStamp)
				return index;
		}
		assert(0);
		return 0;
	}

	/* Gets the current index on mKeyScalings to interpolate to based on the
	current animation time */
	int GetScaleIndex(double animationTime) const
	{
		for (int index = 0; index < m_NumScalings - 1; ++index)
		{
			if (animationTime < m_Scales[index + 1].timeStamp)
				return index;
		}
		assert(0);
		return 0;
	}

private:

	/* Gets normalized value for Lerp & Slerp*/
	double GetScaleFactor(double lastTimeStamp, double nextTimeStamp, double animationTime) const
	{
		double scaleFactor = 0.0;
		double midWayLength = animationTime - lastTimeStamp;
		double framesDiff = nextTimeStamp - lastTimeStamp;
		scaleFactor = midWayLength / framesDiff;
		return scaleFactor;
	}

	/*figures out which position keys to interpolate b/w and performs the interpolation
	and returns the translation matrix*/
	glm::mat4 InterpolatePosition(double animationTime) const
	{
		if (1 == m_NumPositions)
			return glm::translate(glm::mat4(1.0f), m_Positions[0].position);

		int p0Index = GetPositionIndex(animationTime);
		int p1Index = p0Index + 1;
		double scaleFactor = GetScaleFactor(m_Positions[p0Index].timeStamp,
			m_Positions[p1Index].timeStamp, animationTime);
		glm::vec3 finalPosition = glm::mix(m_Positions[p0Index].position,
			m_Positions[p1Index].position, (float)scaleFactor);
		return glm::translate(glm::mat4(1.0f), finalPosition);
	}

	/*figures out which rotations keys to interpolate b/w and performs the interpolation
	and returns the rotation matrix*/
	glm::mat4 InterpolateRotation(double animationTime) const
	{
		if (1 == m_NumRotations)
		{
			auto rotation = glm::normalize(m_Rotations[0].orientation);
			return glm::mat4_cast(rotation);
		}

		int p0Index = GetRotationIndex(animationTime);
		int p1Index = p0Index + 1;
		double scaleFactor = GetScaleFactor(m_Rotations[p0Index].timeStamp, m_Rotations[p1Index].timeStamp, animationTime);
		glm::quat finalRotation = glm::slerp(m_Rotations[p0Index].orientation, m_Rotations[p1Index].orientation, (float)scaleFactor);
		finalRotation = glm::normalize(finalRotation);
		return glm::mat4_cast(finalRotation);
	}

	/*figures out which scaling keys to interpolate b/w and performs the interpolation
	and returns the scale matrix*/
	glm::mat4 InterpolateScaling(double animationTime) const
	{
		if (1 == m_NumScalings)
			return glm::scale(glm::mat4(1.0f), m_Scales[0].scale);

		int p0Index = GetScaleIndex(animationTime);
		int p1Index = p0Index + 1;
		double scaleFactor = GetScaleFactor(m_Scales[p0Index].timeStamp, m_Scales[p1Index].timeStamp, animationTime);
		glm::vec3 finalScale = glm::mix(m_Scales[p0Index].scale, m_Scales[p1Index].scale, (float)scaleFactor);
		return glm::scale(glm::mat4(1.0f), finalScale);
	}
};

class EasyAnimation {
public:
	double m_Duration;
	double m_TicksPerSecond;
	std::vector<EasyBone*> m_Bones;
	AssimpNodeData m_RootNode;
	std::map<std::string, EasyBoneInfo> m_BoneInfoMap;
	std::unordered_map<std::string, EasyBone*> boneLookup;

	EasyAnimation(const aiScene* scene, const aiAnimation* animation, std::map<std::string, EasyBoneInfo>& boneInfoMap, int& boneCount)
	{
		assert(scene && scene->mRootNode);
		m_Duration = animation->mDuration;
		m_TicksPerSecond = animation->mTicksPerSecond;
		ReadHeirarchyData(m_RootNode, scene->mRootNode);
		ReadMissingBones(animation, boneInfoMap, boneCount);
	}

private:
	void ReadMissingBones(const aiAnimation* animation, std::map<std::string, EasyBoneInfo>& boneInfoMap, int& boneCount)
	{
		int size = animation->mNumChannels;

		for (int i = 0; i < size; i++)
		{
			auto channel = animation->mChannels[i];
			std::string boneName = channel->mNodeName.data;

			if (boneInfoMap.find(boneName) == boneInfoMap.end())
			{
				boneInfoMap[boneName].id = boneCount; // adds to base model bones 
				boneCount++;
			}
			EasyBone* bone = new EasyBone(channel->mNodeName.data, boneInfoMap[channel->mNodeName.data].id, channel);
			m_Bones.push_back(bone);
			boneLookup[boneName] = bone;
		}

		m_BoneInfoMap = boneInfoMap; // syncs from base model bones 
	}

	void ReadHeirarchyData(AssimpNodeData& dest, const aiNode* src)
	{
		assert(src);

		dest.name = src->mName.data;
		dest.transformation = AiToGlm(src->mTransformation);
		dest.childrenCount = src->mNumChildren;

		for (size_t i = 0; i < src->mNumChildren; i++)
		{
			AssimpNodeData newData;
			ReadHeirarchyData(newData, src->mChildren[i]);
			dest.children.push_back(newData);
		}
	}

};

class EasyAnimator {
private:
	std::vector<glm::mat4x4> m_FinalBoneMatrices;
	double m_CurrentTime;

	EasyAnimation* m_NextAnimation = nullptr;
	double m_BlendTime = 0.0;
	double m_BlendDuration = 0.0;
	bool m_IsBlending = false;

	float upperBodyBlend = 0.0f; // 0 = run only, 1 = aim upper

public:
	EasyAnimation* m_CurrentAnimation;

	EasyAnimator(EasyAnimation* animation)
	{
		m_CurrentTime = 0.0;
		m_CurrentAnimation = animation;

		m_FinalBoneMatrices.reserve(200);

		for (int i = 0; i < 200; i++)
			m_FinalBoneMatrices.push_back(glm::mat4x4(1.0f));
	}

	void BlendTo(EasyAnimation* next, double duration)
	{
		if (!next || next == m_CurrentAnimation || m_IsBlending)
			return;
		m_NextAnimation = next;
		m_BlendTime = 0.0;
		m_BlendDuration = duration;
		m_IsBlending = true;
	}

	void UpdateLayered(EasyAnimation* lowerAnim, EasyAnimation* upperAnim, bool aiming, double dt)
	{
		if (!lowerAnim || !upperAnim) return;

		// Smoothly change blend factor
		float target = aiming ? 1.0f : 0.0f;
		upperBodyBlend = glm::mix(upperBodyBlend, target, (float)(dt * 8.0)); // smooth

		// Update both animations
		double runTime = fmod(m_CurrentTime * lowerAnim->m_TicksPerSecond, lowerAnim->m_Duration);
		double aimTime = fmod(m_CurrentTime * upperAnim->m_TicksPerSecond, upperAnim->m_Duration);
		m_CurrentTime += dt;

		std::vector<glm::mat4> runBones(200, glm::mat4(1.0f));
		std::vector<glm::mat4> aimBones(200, glm::mat4(1.0f));

		CalculateBoneTransform(lowerAnim, &lowerAnim->m_RootNode, glm::mat4(1.0f), runBones, runTime);
		CalculateBoneTransform(upperAnim, &upperAnim->m_RootNode, glm::mat4(1.0f), aimBones, aimTime);

		// Mix upper/lower bones
		for (auto& [boneName, info] : lowerAnim->m_BoneInfoMap)
		{
			int id = info.id;
			float mask = 0.0f;
			if (auto it = upperBodyMask.find(boneName); it != upperBodyMask.end())
				mask = it->second * upperBodyBlend;

			float lowerW = 1.0f - mask;
			float upperW = mask;

			m_FinalBoneMatrices[id] = runBones[id] * lowerW + aimBones[id] * upperW;
		}
	}

	void UpdateAnimation(double dt)
	{
		if (!m_CurrentAnimation) return;

		m_CurrentTime += m_CurrentAnimation->m_TicksPerSecond * dt;
		m_CurrentTime = fmod(m_CurrentTime, m_CurrentAnimation->m_Duration);

		// Base animation
		std::vector<glm::mat4> baseBones(200, glm::mat4(1.0f));
		CalculateBoneTransform(m_CurrentAnimation, &m_CurrentAnimation->m_RootNode, glm::mat4(1.0f), baseBones);

		if (m_IsBlending && m_NextAnimation)
		{
			m_BlendTime += dt;
			double alpha = std::min(m_BlendTime / m_BlendDuration, 1.0);
			double nextTime = fmod(m_CurrentTime, m_NextAnimation->m_Duration);

			std::vector<glm::mat4> nextBones(200, glm::mat4(1.0f));
			CalculateBoneTransform(m_NextAnimation, &m_NextAnimation->m_RootNode, glm::mat4(1.0f), nextBones, nextTime);

			for (size_t i = 0; i < m_FinalBoneMatrices.size(); ++i)
				m_FinalBoneMatrices[i] = baseBones[i] * (1.0f - (float)alpha) + nextBones[i] * (float)alpha;

			if (alpha >= 1.0)
			{
				m_CurrentAnimation = m_NextAnimation;
				m_NextAnimation = nullptr;
				m_IsBlending = false;
			}
		}
		else
		{
			m_FinalBoneMatrices = baseBones;
		}
	}

	void PlayAnimation(EasyAnimation* pAnimation)
	{
		m_CurrentAnimation = pAnimation;
		m_CurrentTime = 0.0;
	}

	void CalculateBoneTransform(EasyAnimation* animation, const AssimpNodeData* node, const glm::mat4x4& parentTransform, std::vector<glm::mat4>& outMatrices, double customTime = -1.0)
	{
		double time = (customTime >= 0.0) ? customTime : m_CurrentTime;

		EasyBone* Bone = nullptr;
		if (auto it = animation->boneLookup.find(node->name);
			it != animation->boneLookup.end())
			Bone = it->second;

		glm::mat4 nodeTransform = node->transformation;
		if (Bone)
		{
			Bone->Update(time);
			nodeTransform = Bone->GetLocalTransform();
		}

		glm::mat4 globalTransform = parentTransform * nodeTransform;
		auto& boneInfoMap = animation->m_BoneInfoMap;
		if (auto it = boneInfoMap.find(node->name); it != boneInfoMap.end())
			outMatrices[it->second.id] = globalTransform * it->second.offset;

		for (int i = 0; i < node->childrenCount; ++i)
			CalculateBoneTransform(animation, &node->children[i], globalTransform, outMatrices, time);
	}

	const std::vector<glm::mat4x4>& GetFinalBoneMatrices()
	{
		return m_FinalBoneMatrices;
	}
};

class EasyModel {
public:
	class EasyMesh {
	public:
		std::vector<GLuint> indices;
		std::vector<EasyVertex> vertices;
		GLuint texture;
		GLuint vao, vbo, ebo;

		void LoadToGPU()
		{
			glGenVertexArrays(1, &vao);
			glGenBuffers(1, &vbo);
			glGenBuffers(1, &ebo);

			glBindVertexArray(vao);
			glBindBuffer(GL_ARRAY_BUFFER, vbo);
			glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(EasyVertex), vertices.data(), GL_STATIC_DRAW);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(EasyVertex), (void*)0);

			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(EasyVertex), (void*)offsetof(EasyVertex, uv));

			glEnableVertexAttribArray(2);
			glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(EasyVertex), (void*)offsetof(EasyVertex, normal));

			glEnableVertexAttribArray(3);
			glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(EasyVertex), (void*)offsetof(EasyVertex, tangent));

			glEnableVertexAttribArray(4);
			glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(EasyVertex), (void*)offsetof(EasyVertex, bitangent));

			glEnableVertexAttribArray(5);
			glVertexAttribIPointer(5, 4, GL_INT, sizeof(EasyVertex), (void*)offsetof(EasyVertex, boneIds));

			glEnableVertexAttribArray(6);
			glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(EasyVertex), (void*)offsetof(EasyVertex, weights));

			glBindVertexArray(0);
		}
	};
public:
	std::map<std::string, EasyBoneInfo> m_BoneInfoMap;
	std::vector<EasyAnimation*> animations;
	EasyAnimator* animator = nullptr;
	int m_BoneCounter = 0;

	std::vector<EasyMesh*> meshes;

	EasyModel(const std::string file, const std::vector<std::string> animationsStr = {})
	{
		const aiScene* scene = aiImportFile(file.c_str(), aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals | aiProcess_GenBoundingBoxes | aiProcess_CalcTangentSpace);
		assert(scene != nullptr);

		ProcessNode(scene->mRootNode, scene);

		aiReleaseImport(scene);

		for (std::string file : animationsStr)
		{
			const aiScene* animScene = aiImportFile(file.c_str(), aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals | aiProcess_GenBoundingBoxes | aiProcess_CalcTangentSpace);
			assert(animScene != nullptr && animScene->mNumAnimations == 1);
			animations.push_back(new EasyAnimation(animScene, animScene->mAnimations[0], m_BoneInfoMap, m_BoneCounter));
			aiReleaseImport(animScene);
		}
		animator = new EasyAnimator(animations.at(1));
	}

	bool Update(double _dt, bool mb1_pressed)
	{
		EasyAnimation* runAnim = animations.at(1);   // running anim
		EasyAnimation* aimAnim = animations.at(2);   // aiming anim
		animator->UpdateLayered(runAnim, aimAnim, mb1_pressed, _dt);

		return true;
	}

	void LoadToGPU()
	{
		for (auto mesh : meshes)
			mesh->LoadToGPU();
	}

	EasyMesh* ProcessMesh(aiMesh* aiMesh, const aiScene* scene)
	{
		EasyMesh* mesh = new EasyMesh();

		for (size_t v = 0; v < aiMesh->mNumVertices; v++)
		{
			glm::vec3 position = glm::vec3(aiMesh->mVertices[v].x, aiMesh->mVertices[v].y, aiMesh->mVertices[v].z);

			EasyVertex ev;

			ev.position = position;
			if (aiMesh->HasTextureCoords(0))
				ev.uv = { aiMesh->mTextureCoords[0][v].x, aiMesh->mTextureCoords[0][v].y };
			if (aiMesh->HasNormals())
				ev.normal = { aiMesh->mNormals[v].x, aiMesh->mNormals[v].y, aiMesh->mNormals[v].z };
			if (aiMesh->HasTangentsAndBitangents())
				ev.tangent = { aiMesh->mTangents[v].x, aiMesh->mTangents[v].y, aiMesh->mTangents[v].z };
			if (aiMesh->HasTangentsAndBitangents())
				ev.bitangent = { aiMesh->mBitangents[v].x, aiMesh->mBitangents[v].y, aiMesh->mBitangents[v].z };

			mesh->vertices.push_back(ev);
		}

		for (size_t f = 0; f < aiMesh->mNumFaces; f++)
		{
			mesh->indices.push_back((GLuint)aiMesh->mFaces[f].mIndices[0]);
			mesh->indices.push_back((GLuint)aiMesh->mFaces[f].mIndices[1]);
			mesh->indices.push_back((GLuint)aiMesh->mFaces[f].mIndices[2]);
		}

		ExtractBoneWeightForVertices(aiMesh, mesh, scene);

		GLuint texture;
		std::string texPath = "";
		aiString path;
		aiMaterial* material = scene->mMaterials[aiMesh->mMaterialIndex];
		if (material->GetTexture(aiTextureType_DIFFUSE, 0, &path) == AI_SUCCESS)
			texPath = std::string(path.C_Str());
		if (const aiTexture* embedded = scene->GetEmbeddedTexture(texPath.c_str()))
			texture = EasyTexture::Load(embedded);
		mesh->texture = texture;

		return mesh;
	}

	void ProcessNode(const aiNode* node, const aiScene* scene)
	{
		for (unsigned int i = 0; i < node->mNumMeshes; i++)
		{
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
			meshes.push_back(ProcessMesh(mesh, scene));
		}
		for (unsigned int i = 0; i < node->mNumChildren; i++)
		{
			ProcessNode(node->mChildren[i], scene);
		}
	}

	void ExtractBoneWeightForVertices(aiMesh* aiMesh, EasyMesh* mesh, const aiScene* scene)
	{
		for (size_t boneIndex = 0; boneIndex < aiMesh->mNumBones; ++boneIndex)
		{
			int boneID = -1;
			std::string boneName = aiMesh->mBones[boneIndex]->mName.C_Str();
			if (m_BoneInfoMap.find(boneName) == m_BoneInfoMap.end())
			{
				m_BoneInfoMap[boneName] = { m_BoneCounter, AiToGlm(aiMesh->mBones[boneIndex]->mOffsetMatrix) };
				boneID = m_BoneCounter;
				m_BoneCounter++;
			}
			else
			{
				boneID = m_BoneInfoMap[boneName].id;
			}
			assert(boneID != -1);
			auto weights = aiMesh->mBones[boneIndex]->mWeights;
			int numWeights = aiMesh->mBones[boneIndex]->mNumWeights;
			for (int weightIndex = 0; weightIndex < numWeights; ++weightIndex)
			{
				int vertexId = weights[weightIndex].mVertexId;
				float weight = weights[weightIndex].mWeight;
				assert(vertexId <= mesh->vertices.size());
				for (int i = 0; i < 4; ++i)
				{
					if (mesh->vertices[vertexId].boneIds[i] < 0)
					{
						mesh->vertices[vertexId].weights[i] = weight;
						mesh->vertices[vertexId].boneIds[i] = boneID;
						break;
					}
				}
			}
		}

		for (auto& v : mesh->vertices)
		{
			float total = v.weights.x + v.weights.y + v.weights.z + v.weights.w;
			if (total > 0.0f)
				v.weights /= total;      // normalize
			for (int i = 0; i < 4; i++)
				if (v.boneIds[i] >= m_BoneCounter || v.boneIds[i] < 0)
					v.weights[i] = 0.0f;  // safety
		}

		for (auto idx : mesh->indices)
			assert(idx < mesh->vertices.size());
	}
};

class EasyCamera {
public:
	const EasyDisplay& display;

	glm::vec3 position{ 0.f, 0.f, 3.f };
	glm::vec3 front{ 0.f, 0.f, -1.f };
	glm::vec3 up{ 0.f, 1.f, 0.f };
	glm::vec3 right{ 1.f, 0.f, 0.f };
	glm::vec3 worldUp{ 0.f, 1.f, 0.f };
	bool mode = true;

	float yaw = -89.f;
	float pitch = -26.f;

	glm::mat4 view_{ 1.f };
	glm::mat4 projection_{ 1.f };

	float moveSpeed = 5.f;
	float mouseSensitivity = 0.13f;
	float lerpSpeed = 10.f;

	// --- Input state ---
	glm::vec2 mouseDelta{ 0.f };
	float scrollDelta = 0.f;
	std::unordered_map<int, bool> keyState;

	bool firstMouse = true;
	double lastX = 0.0, lastY = 0.0;

	// --- Target states for smoothing ---
	glm::vec3 targetPos;
	glm::vec3 targetFront;

public:
	EasyCamera(const EasyDisplay& display, glm::vec3 pos, glm::vec3 target, float fov, float nearP, float farP) : display(display)
	{
		position = pos;
		front = glm::normalize(target - pos);
		UpdateVectors();

		targetPos = position;
		targetFront = front;

		projection_ = glm::perspective(glm::radians(fov), display.windowSize.x / (float)display.windowSize.y, nearP, farP);
		view_ = glm::lookAt(position, position + front, up);

		ModeSwap();
	}

	void ModeSwap()
	{
		if (mode)
		{
			glfwSetInputMode(display.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			glfwSetInputMode(display.window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
		}
		else
		{
			glfwSetInputMode(display.window, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
			glfwSetInputMode(display.window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}
	}

	// --- Callbacks just record input ---
	void cursor_callback(double xpos, double ypos)
	{
		if (firstMouse) {
			lastX = xpos;
			lastY = ypos;
			firstMouse = false;
		}

		mouseDelta.x += (float)(xpos - lastX);
		mouseDelta.y += (float)(lastY - ypos);
		lastX = xpos;
		lastY = ypos;
	}

	void scroll_callback(double xoffset, double yoffset)
	{
		scrollDelta = (float)yoffset * moveSpeed * 55;
	}

	void key_callback(int key, int scancode, int action, int mods)
	{
		if (action == GLFW_PRESS)
			keyState[key] = true;
		else if (action == GLFW_RELEASE)
			keyState[key] = false;

	}

	void mouse_callback(GLFWwindow* window, int button, int action, int mods)
	{
		if (button == GLFW_MOUSE_BUTTON_2 && action == GLFW_RELEASE)
		{
			mode = !mode;
			ModeSwap();
		}
	}

	// --- Smooth update ---
	void Update(double dt)
	{
		// 1. Handle rotation (from mouse)
		float dx = mouseDelta.x * mouseSensitivity;
		float dy = mouseDelta.y * mouseSensitivity;
		mouseDelta = glm::vec2(0.0f); // reset after use

		yaw += dx;
		pitch += dy;
		pitch = glm::clamp(pitch, -89.0f, 89.0f);

		glm::vec3 newFront;
		newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
		newFront.y = sin(glm::radians(pitch));
		newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
		targetFront = glm::normalize(newFront);

		UpdateVectors();

		// 2. Handle movement (from WASD)
		glm::vec3 move(0.f);
		if (keyState[GLFW_KEY_W]) move += front;
		if (keyState[GLFW_KEY_S]) move -= front;
		if (keyState[GLFW_KEY_A]) move -= right;
		if (keyState[GLFW_KEY_D]) move += right;
		if (keyState[GLFW_KEY_SPACE]) move += up;
		if (keyState[GLFW_KEY_LEFT_SHIFT]) move -= up;

		if (glm::length(move) > 0.0f)
			move = glm::normalize(move);

		// scroll moves forward/back
		move += front * scrollDelta;
		scrollDelta = 0.0f;

		targetPos += move * moveSpeed * (float)dt;

		// 3. Smooth interpolate position & direction
		position = glm::mix(position, targetPos, (float)(dt * lerpSpeed));
		front = glm::normalize(glm::mix(front, targetFront, (float)(dt * lerpSpeed)));

		UpdateVectors();

		view_ = glm::lookAt(position, position + front, up);
	}

private:
	void UpdateVectors()
	{
		right = glm::normalize(glm::cross(front, worldUp));
		up = glm::normalize(glm::cross(right, front));
	}
};

class EasyPlaygrond {
public:
	static inline int exitRequested = 0;
	static inline int animation = 1;
	static inline bool mb1_pressed = false;
	static inline EasyCamera* cameraPtr;
	const EasyDisplay& display_;
	EasyModel model = EasyModel("../../res/Kachujin G Rosales Skin.fbx", { "../../res/Standing Idle on Kachujin G Rosales wo Skin.fbx", "../../res/Running on Kachujin G Rosales wo Skin.fbx", "../../res/Standing Aim Idle 01 on Kachujin H Rosales wo Skin.fbx" });
	//EasyModel sword = EasyModel("../../res/sword.fbx");
	EasyShader shader = EasyShader(VS, FS);
	EasyCamera camera = EasyCamera(display_, { 3,194,166 }, { 3-0.15,194-0.44,166-0.895 }, 74.f, 0.01f, 1000.f);
	double fps = 0.0, ups = 0.0;

	EasyPlaygrond(const EasyDisplay& display) : display_(display)
	{

	}

	bool Init()
	{
		glfwSetKeyCallback(display_.window, EasyPlaygrond::key_callback);
		glfwSetCursorPosCallback(display_.window, EasyPlaygrond::cursor_callback);
		glfwSetMouseButtonCallback(display_.window, EasyPlaygrond::mouse_callback);
		glfwSetScrollCallback(display_.window, EasyPlaygrond::scroll_callback);

		model.LoadToGPU();
		//sword.LoadToGPU();

		shader.Start();
		shader.BindAttribs({ "position", "uv", "normal", "tangent", "bitangent", "boneIds", "weights" });
		shader.BindUniforms({ "diffuse", "view", "proj", "model", "animated" });
		shader.BindUniformArray("boneMatrices", 200);
		shader.Stop();

		cameraPtr = &camera;

		return true;
	}

	bool Update(double _dt)
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

	bool Render(double _dt)
	{
		//if (model.animator)
		//	model.animator->BlendTo(model.animations.at(animation ? 1 : 0), 1.0);

		model.Update(_dt, mb1_pressed);

		camera.Update(_dt);

		bool success = true;

		// Early Render
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

		// Render
		{
			shader.Start();

			shader.LoadUniform("view", camera.view_);
			shader.LoadUniform("proj", camera.projection_);

			std::vector<EasyModel*> models = {  &model };
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

		// End Render
		{
			glfwSetWindowTitle(display_.window, (std::ostringstream() << std::fixed << std::setprecision(3) << "FPS: " << fps << " | UPS: " << ups).str().c_str());
			glfwSwapBuffers(display_.window);
			glfwPollEvents();
		}

		return success;
	}

	static void scroll_callback(GLFWwindow* window, double xpos, double ypos)
	{
		cameraPtr->scroll_callback(xpos, ypos);
	}

	static void cursor_callback(GLFWwindow* window, double xpos, double ypos)
	{
		cameraPtr->cursor_callback(xpos, ypos);
	}

	static void mouse_callback(GLFWwindow* window, int button, int action, int mods)
	{
		if (button == GLFW_MOUSE_BUTTON_1 && action == GLFW_PRESS)
			mb1_pressed = true;
		else if (button == GLFW_MOUSE_BUTTON_1 && action == GLFW_RELEASE)
			mb1_pressed = false;
		cameraPtr->mouse_callback(window, button, action, mods);
	}

	static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		cameraPtr->key_callback(key, scancode, action, mods);

		if (key == GLFW_KEY_ESCAPE && action == GLFW_RELEASE)
		{
			exitRequested = 1;
		}

		if (key == GLFW_KEY_T && action == GLFW_RELEASE)
		{
			animation = 2;
		}
	}

	bool OneLoop()
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
			success &= Render(fps_timer / 1000.0);
			fps_timer = 0.0;
		}

		if (ups_timer >= ups_constant)
		{
			success &= Update(ups_timer / 1000.0);
			ups_timer = 0.0;
		}

		success &= (glfwWindowShouldClose(display_.window) == 0);

		return success;
	}

};

int main()
{
	if (EasyDisplay display; display.Init())
	{
		if (EasyPlaygrond playground(display); playground.Init())
		{
			while (playground.OneLoop());
		}
	}
	return 0;
}

#endif