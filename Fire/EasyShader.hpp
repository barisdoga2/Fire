#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>
#include <glad/glad.h> 



class EasyShader {
public:
    GLuint prog = 0;
    std::unordered_map<std::string, GLint> attribs;
    std::unordered_map<std::string, GLint> uniforms;

    EasyShader(const std::string& name);
    EasyShader(const char* VS, const char* FS);
    void Start();
    void Stop();

    void BindAttribs(const std::vector<std::string>& attribs);
    void BindUniforms(const std::vector<std::string>& uniforms);
    void BindUniformArray(std::string name, int size);

    void LoadUniform(const std::string& uniform, const glm::vec3& value);
    void LoadUniform(const std::string& uniform, const GLint& value);
    void LoadUniform(const std::string& uniform, const glm::mat4x4& mat4x4);
    void LoadUniform(const std::string& uniform, const std::vector<glm::mat4x4>& mat4x4);

private:
    void AttachShader(GLenum type, const char* src, const GLint* length = nullptr);

};
