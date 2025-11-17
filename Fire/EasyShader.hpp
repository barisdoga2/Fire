#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>
#include <glad/glad.h> 

#define GENERAL_UNIFORMS {"uCameraPos", "uIsFog"}


class EasyShader {
public:
    GLuint prog = 0;
    std::unordered_map<std::string, GLint> attribs;
    std::unordered_map<std::string, GLint> uniforms;

    EasyShader(const std::string& name);
    EasyShader(const char* VSs, const char* FSs, const char* GSs = nullptr);
    void Start();
    void Stop();

    void BindAttribs(const std::vector<std::string>& attribs);
    void BindTextures(const std::vector<std::string>& textures);
    void BindUniforms(const std::vector<std::string>& uniforms);
    void BindUniformArray(std::string name, int size);

    void LoadTexture(GLint slot, GLenum type, std::string name, GLuint texture);
    void LoadUniform(const std::string& uniform, const glm::vec3& value);
    void LoadUniform(const std::string& uniform, const glm::vec4& value);
    void LoadUniform(const std::string& uniform, const GLint& value);
    void LoadUniform(const std::string& uniform, const glm::mat4x4& mat4x4);
    void LoadUniform(const std::string& uniform, const std::vector<glm::mat4x4>& mat4x4);
    void LoadUniform(const std::string& uniform, const GLfloat& value);
    void LoadUniform(const std::string& uniform, const unsigned int& value);

private:
    void AttachShader(GLenum type, const char* src, const GLint* length = nullptr);

};
