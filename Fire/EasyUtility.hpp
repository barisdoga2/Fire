#pragma once

#include <vector>
#include <string>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <assimp/scene.h>



static inline glm::mat4 AiToGlm(const aiMatrix4x4& m)
{
    glm::mat4 result;
    result[0][0] = m.a1; result[1][0] = m.a2; result[2][0] = m.a3; result[3][0] = m.a4;
    result[0][1] = m.b1; result[1][1] = m.b2; result[2][1] = m.b3; result[3][1] = m.b4;
    result[0][2] = m.c1; result[1][2] = m.c2; result[2][2] = m.c3; result[3][2] = m.c4;
    result[0][3] = m.d1; result[1][3] = m.d2; result[2][3] = m.d3; result[3][3] = m.d4;
    return result;
}

static inline glm::vec3 AiToGlm(const aiVector3D& m)
{
    glm::vec3 result;
    result.x = m.x;
    result.y = m.y;
    result.z = m.z;
    return result;
}

static inline glm::quat AiToGlm(const aiQuaternion& q)
{
    return glm::quat(q.w, q.x, q.y, q.z);
}

static inline glm::mat4 CreateTransformMatrix(const glm::vec3& position, const glm::vec3& rotationDeg, const glm::vec3& scale)
{
    // Convert degrees to radians
    glm::vec3 rotation = glm::radians(rotationDeg);

    // Build individual matrices
    glm::mat4 translation = glm::translate(glm::mat4(1.0f), position);
    glm::mat4 rotationMat = glm::yawPitchRoll(rotation.y, rotation.x, rotation.z);
    glm::mat4 scaling = glm::scale(glm::mat4(1.0f), scale);

    // Combine (T * R * S)
    glm::mat4 transform = translation * rotationMat * scaling;

    return transform;
}

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
