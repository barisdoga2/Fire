#pragma once

#include <vector>
#include <string>
#include <fstream>
#include <chrono>

#include <assimp/scene.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>



struct AssimpNodeData {
    glm::mat4x4 transformation;
    std::string name;
    int childrenCount = 0;
    std::vector<AssimpNodeData> children;
};

struct EasyVertex {
    glm::vec3 position = glm::vec3(0);
    glm::vec2 uv = glm::vec2(0);
    glm::vec3 normal = glm::vec3(0);
    glm::vec3 tangent = glm::vec3(0);
    glm::vec3 bitangent = glm::vec3(0);
    glm::ivec4 boneIds = glm::ivec4(-1);
    glm::vec4 weights = glm::vec4(0);
};

struct EasyBoneInfo {
    int id = -1;
    glm::mat4 offset = glm::mat4(1.0f);
};

struct KeyPosition {
    glm::vec3 position;
    double timeStamp = 0.0;
};

struct KeyRotation {
    glm::quat orientation;
    double timeStamp = 0.0;
};

struct KeyScale {
    glm::vec3 scale;
    double timeStamp = 0.0;
};


float Noise2D(float x, float z, float scale, int seed);

float FractalNoise2D(float x, float z, float scale, int octaves, float persistence, int seed);

std::string LoadFile(std::string file);

glm::mat4 AiToGlm(const aiMatrix4x4& m);

glm::vec3 AiToGlm(const aiVector3D& m);

glm::quat AiToGlm(const aiQuaternion& q);

glm::mat4 CreateTransformMatrix(const glm::vec3& position, const glm::vec3& rotationDeg, const glm::vec3& scale);

glm::mat4 CreateTransformMatrix(const glm::vec3& position, const glm::quat& rotationQuat, const glm::vec3& scale);

glm::mat4 ConvertMatrixToGLMFormat(const aiMatrix4x4& from);

unsigned int CreateCube3D(float size, unsigned int* vbo, float* positions_out);

std::string HashSHA256(const std::string& input);

std::string TimeNow_HHMMSS();

bool LoadFileBinary(std::string file, std::vector<char>* out);

void EasyUtils_Init();

void ToggleConsole();

void ShowConsole(bool show = true);

std::string GetRelPath(std::string append);
