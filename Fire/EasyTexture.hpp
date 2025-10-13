#pragma once

#include <string>

#include <glad/glad.h>



struct aiTexture;
class EasyTexture {
public:
    static GLuint Load(const std::string& path);
    static GLuint Load(const aiTexture* embedded);

};
