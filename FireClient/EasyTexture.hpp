#pragma once

#include <string>
#include <atomic>

#include <glad/glad.h>



struct aiTexture;
class EasyTexture {
public:
    int width{}, height{}, channels{};
    GLuint textureID{};
    std::string path{};
    unsigned char* data{};
    std::atomic<bool> isDataReady{};
    bool isReady{};
    bool doNotFree{};
    EasyTexture(std::string path);
    EasyTexture(std::string path, const aiTexture* texture);
    bool LoadToGPU();

};
