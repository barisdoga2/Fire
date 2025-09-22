#pragma once

#include <map>
#include <unordered_map>
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <glad/glad.h>



class Shader {
public:
	GLint prog;
	std::unordered_map<std::string, GLint> uniforms;
	std::vector<std::string> attribs;
	std::string name;

	Shader(std::string name, std::vector<std::string> attribs);
	Shader(char* ptr, std::vector<std::string> attribs);
	Shader(const char* vertexShaderSource, const char* fragmentShaderSource, std::vector<std::string> attribs);
	~Shader();

	void Use();
	void Start();
	void Stop();

	void BindTexture(GLint slot, GLenum type, std::string name, GLuint texture);

	void ConnectUniform(std::string uniform);

	void ConnectUniforms(std::vector<std::string> uniformsToBind);

	void ConnectUniformStruct(std::string name, const std::vector<std::string>& variables);

	void ConnectUniformStructArray(std::string name, int size, const std::vector<std::string>& variables);

	void ConnectUniformArray(std::string name, int size);

	void ConnectTextureUnits(std::vector<std::string> textures);

	inline void LoadUniformArray(const std::string& uniform, const std::vector<float>& vec, const unsigned int& max = 0)
	{
		if (max == 0)
		{
			for (unsigned int i = 0; i < vec.size(); i++)
				LoadUniform(uniform + "[" + std::to_string(i) + "]", vec.at(i));
		}
		else
		{
			for (unsigned int i = 0; i < vec.size() && i < max; i++)
				LoadUniform(uniform + "[" + std::to_string(i) + "]", vec.at(i));
		}
	}

	inline void LoadUniformArray(const std::string& uniform, const std::vector<glm::vec3>& vec, const unsigned int& max = 0)
	{
		if (max == 0)
		{
			for (unsigned int i = 0; i < vec.size(); i++)
				LoadUniform(uniform + "[" + std::to_string(i) + "]", vec.at(i));
		}
		else
		{
			for (unsigned int i = 0; i < vec.size() && i < max; i++)
				LoadUniform(uniform + "[" + std::to_string(i) + "]", vec.at(i));
		}
	}

	inline void LoadUniformArray(const std::string& uniform, const std::vector<glm::mat4x4>& vec, const unsigned int& max = 0)
	{
		if (max == 0)
		{
			for (unsigned int i = 0; i < vec.size(); i++)
				LoadUniform(uniform + "[" + std::to_string(i) + "]", vec.at(i));
		}
		else
		{
			for (unsigned int i = 0; i < vec.size() && i < max; i++)
				LoadUniform(uniform + "[" + std::to_string(i) + "]", vec.at(i));
		}
	}

	inline void LoadUniform(const std::string& uniform, const glm::mat4x4& mat4x4)
	{
		glUniformMatrix4fv(uniforms[uniform], 1, GL_FALSE, (GLfloat*)&mat4x4[0][0]);
	}

	inline void LoadUniform(const std::string& uniform, const float* mat4x4)
	{
		glUniformMatrix4fv(uniforms[uniform], 1, GL_FALSE, (GLfloat*)mat4x4);
	}

	inline void LoadUniform(const std::string& uniform, const GLboolean& val)
	{
		glUniform1i(uniforms[uniform], (int)val);
	}

	inline void LoadUniform(const std::string& uniform, const  GLuint& val)
	{
		glUniform1ui(uniforms[uniform], val);
	}

	inline void LoadUniform(const std::string& uniform, const GLint& val)
	{
		glUniform1i(uniforms[uniform], val);
	}

	inline void LoadUniform(const std::string& uniform, const GLfloat& val)
	{
		glUniform1f(uniforms[uniform], val);
	}

	inline void LoadUniform(const std::string& uniform, const glm::vec3& val)
	{
		glUniform3f(uniforms[uniform], val.x, val.y, val.z);
	}

	inline void LoadUniform(const std::string& uniform, const glm::vec4& val)
	{
		glUniform4f(uniforms[uniform], val.x, val.y, val.z, val.w);
	}

	inline void LoadUniform(const std::string& uniform, const glm::vec2& val)
	{
		glUniform2f(uniforms[uniform], val.x, val.y);
	}
protected:

	bool CheckStatus(GLuint obj, bool isShader);

	void AttachShader(GLenum type, const char* src, const GLint* length = nullptr);
};
