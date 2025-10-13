#pragma once

#include <glm/glm.hpp>



struct GLFWwindow;
class EasyDisplay {
public:
    glm::tvec2<int> windowSize = { 1920, 1080 };
    GLFWwindow* window = nullptr;

    EasyDisplay(glm::tvec2<int> windowSize = { 800, 600 });
    ~EasyDisplay();

    bool Init();

};