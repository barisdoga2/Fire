#include "EasyTexture.hpp"

#include <iostream>
#include <cassert>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <assimp/scene.h>



GLuint EasyTexture::Load(const std::string& path)
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

GLuint EasyTexture::Load(const aiTexture* embedded)
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
