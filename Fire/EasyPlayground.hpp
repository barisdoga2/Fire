#pragma once

#include "EasyCamera.hpp"
#include "EasyShader.hpp"
#include "EasyModel.hpp"
#include "EasyAnimator.hpp"
#include "EasyDisplay.hpp"


class EasyPlayground {
private:
public:
    int exitRequested = 0;
    int animation = 1;
    bool mb1_pressed = false;
    const EasyDisplay& display_;
    EasyModel model = EasyModel("../../res/Kachujin G Rosales Skin.fbx", { "../../res/Standing Idle on Kachujin G Rosales wo Skin.fbx", "../../res/Running on Kachujin G Rosales wo Skin.fbx", "../../res/Standing Aim Idle 01 on Kachujin H Rosales wo Skin.fbx" });
    EasyShader shader = EasyShader("model");
    EasyCamera camera = EasyCamera(display_, { 3,194,166 }, { 3 - 0.15,194 - 0.44,166 - 0.895 }, 74.f, 0.01f, 1000.f);
    double fps = 0.0, ups = 0.0;

    EasyPlayground(const EasyDisplay& display);
    ~EasyPlayground();

    bool Init();

    bool Update(double _dt);
    bool Render(double _dt);

    bool OneLoop();

    void scroll_callback(GLFWwindow* window, double xpos, double ypos);
    void cursor_callback(GLFWwindow* window, double xpos, double ypos);
    void mouse_callback(GLFWwindow* window, int button, int action, int mods);
    void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
    void char_callback(GLFWwindow* window, unsigned int codepoint);

};
