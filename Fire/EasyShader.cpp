#include "EasyShader.hpp"
#include "GL_Ext.hpp"
#include "EasyUtility.hpp"

#include <iostream>



EasyShader::EasyShader(const std::string& name)
{
    prog = glCreateProgram();

    std::string vertexShaderSource = LoadFile(GetPath("res/shaders/") + name + "V.glsl");
    if (vertexShaderSource.length() > 0)
        AttachShader(GL_VERTEX_SHADER, vertexShaderSource.c_str());

    std::string fragmentShaderSource = LoadFile(GetPath("res/shaders/") + name + "F.glsl");
    if (fragmentShaderSource.length() > 0)
        AttachShader(GL_FRAGMENT_SHADER, fragmentShaderSource.c_str());

    std::string geometryShaderSource = LoadFile(GetPath("res/shaders/") + name + "G.glsl");
    if (geometryShaderSource.length() > 0)
        AttachShader(GL_GEOMETRY_SHADER, geometryShaderSource.c_str());

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

EasyShader::EasyShader(const char* VSs, const char* FSs, const char* GSs)
{
    prog = glCreateProgram();

    AttachShader(GL_VERTEX_SHADER, VSs);

    AttachShader(GL_FRAGMENT_SHADER, FSs);

    if(GSs)
        AttachShader(GL_GEOMETRY_SHADER, GSs);

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

void EasyShader::Start()
{
    glUseProgram(prog);
}

void EasyShader::Stop()
{
    glUseProgram(0);
}

void EasyShader::BindAttribs(const std::vector<std::string>& attribs)
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

void EasyShader::BindUniforms(const std::vector<std::string>& uniforms)
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

void EasyShader::BindTextures(const std::vector<std::string>& textures)
{
    for (unsigned int i = 0; i < textures.size(); i++)
    {
        GLint loc_1;
        loc_1 = glGetUniformLocation(prog, textures.at(i).c_str());
        uniforms.insert(std::pair<std::string, GLint> {textures.at(i), loc_1});
        LoadUniform(textures.at(i), (int)i);
    }
}

void EasyShader::BindUniformArray(std::string name, int size)
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

void EasyShader::LoadUniform(const std::string& uniform, const glm::vec3& value)
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

void EasyShader::LoadUniform(const std::string& uniform, const glm::vec4& value)
{
    auto res = uniforms.find(uniform);
    if (res != uniforms.end())
    {
        GL(Uniform4f(res->second, value.x, value.y, value.z, value.w));
    }
    else
    {
        std::cout << "Shader uniform to load not found! " << uniform << std::endl;
    }
}

void EasyShader::LoadUniform(const std::string& uniform, const GLfloat& value)
{
    auto res = uniforms.find(uniform);
    if (res != uniforms.end())
    {
        GL(Uniform1f(res->second, value));
    }
    else
    {
        std::cout << "Shader uniform to load not found! " << uniform << std::endl;
    }
}

void EasyShader::LoadUniform(const std::string& uniform, const GLint& value)
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

void EasyShader::LoadUniform(const std::string& uniform, const unsigned int& value)
{
    auto res = uniforms.find(uniform);
    if (res != uniforms.end())
    {
        GL(Uniform1ui(res->second, value));
    }
    else
    {
        std::cout << "Shader uniform to load not found! " << uniform << std::endl;
    }
}

void EasyShader::LoadUniform(const std::string& uniform, const glm::mat4x4& value)
{
    auto res = uniforms.find(uniform);
    if (res != uniforms.end())
    {
        GL(UniformMatrix4fv(res->second, 1, GL_FALSE, (GLfloat*)&value[0][0]));
    }
    else
    {
        std::cout << "Shader uniform to load not found! " << uniform << std::endl;
    }
}

void EasyShader::LoadUniform(const std::string& uniform, const std::vector<glm::mat4x4>& value)
{
    auto res = uniforms.find(uniform + "[0]");
    if (res != uniforms.end())
    {
        GL(UniformMatrix4fv(res->second, (GLsizei)value.size(), GL_FALSE, (GLfloat*)value.data()));
    }
    else
    {
        std::cout << "Shader uniform array to load not found! " << uniform << std::endl;
    }
}

void EasyShader::LoadTexture(GLint slot, GLenum type, std::string name, GLuint texture)
{
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(type, texture);
    LoadUniform(name, slot);
}

void EasyShader::AttachShader(GLenum type, const char* src, const GLint* length)
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
