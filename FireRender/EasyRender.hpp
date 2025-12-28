#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include "EasyUtils.hpp"

struct RenderData {
	bool imgui_isFog{};
	bool imgui_triangles{};
	bool imgui_showNormalLines{};
	float imgui_showNormalLength = 2.2f;
};

class EasyTransform {
public:
    glm::vec3 position = glm::vec3(0, 0, 0);
    glm::vec3 scale = glm::vec3(1, 1, 1);
    glm::quat rotationQuat = glm::quat(1, 0, 0, 0);

    EasyTransform(glm::vec3 position = glm::vec3(0, 0, 0), glm::quat rotationQuat = glm::quat(1, 0, 0, 0), glm::vec3 scale = glm::vec3(1, 1, 1)) 
        : position(position), rotationQuat(rotationQuat), scale(scale)
    {
        
    }

    EasyTransform(glm::mat4x4 mat)
        : position(), rotationQuat(), scale()
    {
        glm::vec3 skew{};
        glm::vec4 perspective{};
        glm::decompose(mat, scale, rotationQuat, position, skew, perspective);
    }

    const glm::mat4x4 TransformationMatrix()
    {
        return CreateTransformMatrix(position, rotationQuat, scale);
    }
};

class EasyTransformExt : public EasyTransform {
public:
    glm::vec3 skew{};
    glm::vec4 perspective{};

    EasyTransformExt(glm::vec3 position = glm::vec3(0, 0, 0), glm::quat rotationQuat = glm::quat(1, 0, 0, 0), glm::vec3 scale = glm::vec3(1, 1, 1)) 
        : EasyTransform(position, rotationQuat, scale)
    {
        
    }

    EasyTransformExt(glm::mat4x4 mat)
        : EasyTransform()
    {
        glm::decompose(mat, scale, rotationQuat, position, skew, perspective);
    }

};

