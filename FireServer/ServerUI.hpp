#pragma once

#include <vector>
#include <string>

class FireServer;
class EasyDisplay;
struct GLFWwindow;
class ServerUI {
private:
    enum CommandType {
        CONSOLE_COMMAND,
        BROADCAST_COMMAND,
        SHUTDOWN_COMMAND
    };

public:
    EasyDisplay* display;
    FireServer* server;
    double fps = 0.0, ups = 0.0;

    ServerUI(EasyDisplay* display, FireServer* server);
    ~ServerUI();

    bool Init();

    bool Update(double _dt);
    bool Render(double _dt);
    void StartRender(double _dt);
    void EndRender();

    void OnCommand(CommandType type, std::string command);

    void ImGUI_DrawConsoleWindow();
    void ImGUI_DrawManagementWindow();
    void ImGUI_DrawSessionsWindow();

    void scroll_callback(GLFWwindow* window, double xpos, double ypos);
    void cursor_callback(GLFWwindow* window, double xpos, double ypos);
    void mouse_callback(GLFWwindow* window, int button, int action, int mods);
    void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
    void char_callback(GLFWwindow* window, unsigned int codepoint);

    static void ForwardStandartIO();

};
