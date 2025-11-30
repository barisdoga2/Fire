#pragma once

#include <glm/glm.hpp>



struct GLFWwindow;
class EasyDisplay {
public:
    glm::tvec2<int> windowSize = { 1920, 1080 };
    glm::tvec2<int> position = { 0, 0 };
    GLFWwindow* window = nullptr;

    EasyDisplay(glm::tvec2<int> windowSize = { 800, 600 }, glm::tvec2<int> position = { 0, 0 });
    ~EasyDisplay();

    bool Init();

    bool ShouldClose();

};