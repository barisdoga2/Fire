#include <iostream>
#include "Shader.hpp"
#include "Utils.hpp"


Shader::Shader(std::string name, std::vector<std::string> attribs)
{
	this->attribs = attribs;
	this->name = name;

	prog = glCreateProgram();

	std::string vertexShaderSource = LoadFile("..\\..\\res\\" + name + "V.glsl");
	if (vertexShaderSource.length() > 0)
		AttachShader(GL_VERTEX_SHADER, vertexShaderSource.c_str());

	std::string fragmentShaderSource = LoadFile("..\\..\\res\\" + name + "F.glsl");
	if (fragmentShaderSource.length() > 0)
		AttachShader(GL_FRAGMENT_SHADER, fragmentShaderSource.c_str());

	for (unsigned int i = 0; i < attribs.size(); i++)
		glBindAttribLocation(prog, i, attribs.at(i).c_str());
	glLinkProgram(prog);
	CheckStatus(prog, false);
	glUseProgram(prog);
}

Shader::Shader(char* ptr, std::vector<std::string> attribs)
{
	this->attribs = attribs;
	this->name = name;

	prog = glCreateProgram();

	size_t iter = 0;

	size_t vertexSize = *((size_t*)(ptr + iter));
	iter += sizeof(size_t);

	size_t fragmentSize = *((size_t*)(ptr + iter));
	iter += sizeof(size_t);

	size_t geometrySize = *((size_t*)(ptr + iter));
	iter += sizeof(size_t);

	size_t tesEvSize = *((size_t*)(ptr + iter));
	iter += sizeof(size_t);

	size_t tesCntSize = *((size_t*)(ptr + iter));
	iter += sizeof(size_t);

	char* vertexSrc = ptr + iter;
	iter += vertexSize;

	char* fragmentSrc = ptr + iter;
	iter += fragmentSize;

	char* geometrySrc = ptr + iter;
	iter += geometrySize;

	char* tesEvSrc = ptr + iter;
	iter += tesEvSize;

	char* tesCntSrc = ptr + iter;
	iter += tesCntSize;

	if (vertexSize > 0)
		AttachShader(GL_VERTEX_SHADER, vertexSrc, (const GLint*)&vertexSize);

	if (fragmentSize > 0)
		AttachShader(GL_FRAGMENT_SHADER, fragmentSrc, (const GLint*)&fragmentSize);

	if (geometrySize > 0)
		AttachShader(GL_GEOMETRY_SHADER, geometrySrc, (const GLint*)&geometrySize);

	if (tesCntSize > 0)
		AttachShader(GL_TESS_CONTROL_SHADER, tesCntSrc, (const GLint*)&tesCntSize);

	if (tesEvSize > 0)
		AttachShader(GL_TESS_EVALUATION_SHADER, tesEvSrc, (const GLint*)&tesEvSize);

	for (unsigned int i = 0; i < attribs.size(); i++)
		glBindAttribLocation(prog, i, attribs.at(i).c_str());
	glLinkProgram(prog);
	CheckStatus(prog, false);
	glUseProgram(prog);
}

Shader::Shader(const char* vertexShaderSource, const char* fragmentShaderSource, std::vector<std::string> attribs)
{
	prog = glCreateProgram();

	AttachShader(GL_VERTEX_SHADER, vertexShaderSource, NULL);

	AttachShader(GL_FRAGMENT_SHADER, fragmentShaderSource, NULL);

	for (unsigned int i = 0; i < attribs.size(); i++)
		glBindAttribLocation(prog, i, attribs.at(i).c_str());
	glLinkProgram(prog);
	CheckStatus(prog, false);
	glUseProgram(prog);
}

Shader::~Shader()
{
	glDeleteProgram(prog);
}

void Shader::Use()
{
	Start();
}

void Shader::Start()
{
	glUseProgram(prog);
}

void Shader::Stop()
{
	glUseProgram(0);
}

void Shader::BindTexture(GLint slot, GLenum type, std::string name, GLuint texture)
{
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(type, texture);
	LoadUniform(name, slot);
}

void Shader::ConnectUniform(std::string uniform)
{
	uniforms.insert(std::pair<std::string, GLint> {uniform, glGetUniformLocation(prog, uniform.c_str())});
}

void Shader::ConnectUniforms(std::vector<std::string> uniformsToBind)
{
	for (auto it : uniformsToBind)
		ConnectUniform(it);
}

void Shader::ConnectUniformStruct(std::string name, const std::vector<std::string>& variables)
{
	for (std::string s : variables)
	{
		GLint loc_1;
		loc_1 = glGetUniformLocation(prog, std::string(name + "." + s).c_str());
		uniforms.insert(std::pair<std::string, GLint> {std::string(name + "." + s), loc_1});
	}
}

void Shader::ConnectUniformStructArray(std::string name, int size, const std::vector<std::string>& variables)
{
	for (int i = 0; i < size; i++)
	{
		for (std::string s : variables)
		{
			GLint loc_1;
			loc_1 = glGetUniformLocation(prog, std::string(name + "[" + std::to_string(i) + "]." + s).c_str());
			uniforms.insert(std::pair<std::string, GLint> {std::string(name + "[" + std::to_string(i) + "]." + s), loc_1});
		}
	}
}

void Shader::ConnectUniformArray(std::string name, int size)
{
	for (int i = 0; i < size; i++)
	{
		GLint loc_1;
		loc_1 = glGetUniformLocation(prog, std::string(name + "[" + std::to_string(i) + "]").c_str());
		uniforms.insert(std::pair<std::string, GLint> {std::string(name + "[" + std::to_string(i) + "]"), loc_1});
	}
}

void Shader::ConnectTextureUnits(std::vector<std::string> textures)
{
	for (unsigned int i = 0; i < textures.size(); i++)
	{
		GLint loc_1;
		loc_1 = glGetUniformLocation(prog, textures.at(i).c_str());
		uniforms.insert(std::pair<std::string, GLint> {textures.at(i), loc_1});
		LoadUniform(textures.at(i), (int)i);
	}
}

bool Shader::CheckStatus(GLuint obj, bool isShader)
{
	GLint status = GL_FALSE, log[1 << 11] = { 0 };
	(isShader ? glGetShaderiv : glGetProgramiv)(obj, isShader ? GL_COMPILE_STATUS : GL_LINK_STATUS, &status);
	if (status == GL_TRUE) return true;
	(isShader ? glGetShaderInfoLog : glGetProgramInfoLog)(obj, sizeof(log), NULL, (GLchar*)log);
	std::cout << name << " - " << (std::string((GLchar*)log).length() == 0 ? "Error Loading Shader!" : (GLchar*)log) << std::endl;
	return false;
}

void Shader::AttachShader(GLenum type, const char* src, const GLint* length)
{
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &src, length);
	glCompileShader(shader);
	CheckStatus(shader, true);
	glAttachShader(prog, shader);
	glDeleteShader(shader);
}
