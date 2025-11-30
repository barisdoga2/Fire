#include "pch.h"
#include "EasyTexture.hpp"

#include <iostream>
#include <cassert>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <assimp/scene.h>

EasyTexture::EasyTexture(std::string path) : path(path)
{
    unsigned char* data_loc = stbi_load(path.c_str(), &width, &height, &channels, 0);
    if (data_loc)
    {
        data = (unsigned char*)malloc(width * height * channels);
        if(data)
            memcpy(data, data_loc, width * height * channels);
    }

    isDataReady.store(true);
    stbi_image_free(data_loc);
}

EasyTexture::EasyTexture(std::string path, const aiTexture* embedded) : path(path)
{
    unsigned char* data_loc;
    if (embedded->mHeight == 0)
    {
        data_loc = stbi_load_from_memory(reinterpret_cast<unsigned char*>(embedded->pcData), embedded->mWidth, &width, &height, &channels, 0);
    }
    else
    {
        width = embedded->mWidth;
        height = embedded->mHeight;
        channels = 4;
        data_loc = reinterpret_cast<unsigned char*>(embedded->pcData);
    }

    if (data_loc)
    {
        data = (unsigned char*)malloc(width * height * channels);
        if (data)
            memcpy(data, data_loc, width * height * channels);
    }

    if (embedded->mHeight != 0)
        stbi_image_free(data_loc);

    isDataReady.store(true);
}

bool EasyTexture::LoadToGPU()
{
    if (isDataReady.load() && !isReady)
    {
        std::cout << "Loading texture '" << path << "'\n";

        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        GLenum format = (channels == 4 ? GL_RGBA : GL_RGB);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);

        free(data);
        data = nullptr;
        isReady = true;
    }
    return isReady;
}

