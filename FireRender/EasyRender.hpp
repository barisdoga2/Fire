#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

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
    glm::vec3 rotation = glm::vec3(0, 0, 0);
    glm::quat rotationQuat = glm::quat(1, 0, 0, 0);

    EasyTransform(glm::vec3 position, glm::vec3 rotation, glm::vec3 scale) : position(position), rotation(rotation), scale(scale)
    {

    }

    EasyTransform(glm::vec3 position, glm::quat rotationQuat, glm::vec3 scale) : position(position), rotationQuat(rotationQuat), scale(scale)
    {

    }

    EasyTransform()
    {

    }
};