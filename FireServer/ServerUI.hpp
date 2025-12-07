#pragma once

#include <vector>
#include <EasyDisplay.hpp>

class ServerUI {
private:
public:
    int exitRequested = 0;
    const EasyDisplay& display_;
    double fps = 0.0, ups = 0.0;

    ServerUI(const EasyDisplay& display);
    ~ServerUI();

    bool Init();

    bool Update(double _dt);
    bool Render(double _dt);
    void StartRender(double _dt);
    void EndRender();

    //void OnSessionListUpdated(const std::vector<Session*>& list);
    void ImGUI_DrawSessionDetailWindow();
    void ImGUI_DrawSessionListWindow();

    void scroll_callback(GLFWwindow* window, double xpos, double ypos);
    void cursor_callback(GLFWwindow* window, double xpos, double ypos);
    void mouse_callback(GLFWwindow* window, int button, int action, int mods);
    void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
    void char_callback(GLFWwindow* window, unsigned int codepoint);

};
