#pragma once

#include "EasyNet.hpp"



class EasyDisplay;
class EasyModel;
class EasyShader;
class EasyCamera;
class HDR;
class Chunk;
class EasyBufferManager;
class ClientNetwork;
struct GLFWwindow;
class EasyPlayground {
private:
public:
    // Display
    EasyDisplay* display{};
    double fps{};
    double ups{};

    // Network
    EasyBufferManager* bm{};
    ClientNetwork* network;
    std::thread loginThread{};

    // Shaders, Renderers
    EasyShader* shader{};
    EasyShader* normalLinesShader{};
    EasyCamera* camera{};
    HDR* hdr{};

    // Base Assets
    EasyModel* model{};
    EasyModel* cube_1x1x1{};
    EasyModel* items{};
    EasyModel* buildings{};
    EasyModel* walls{};

    // Asset Colections
    std::vector<Chunk*> chunks{};
    std::vector<EasyModel*> mapObjects{};

    // Flags
    bool mb1_pressed{};
    bool imgui_triangles{};
    bool imgui_isFog{};
    bool imgui_showNormalLines{};
    bool isRender{};
    bool rememberMe{};
    int animation = 1;
    float imgui_showNormalLength = 2.2f;
    int seed = 1337;

    EasyPlayground(EasyDisplay* display, EasyBufferManager* bm);
    ~EasyPlayground();

    bool Init();

    bool Update(double _dt);
    bool Render(double _dt);
    void StartRender(double _dt);
    void EndRender();
    void ImGUIRender();
    void ReloadShaders();
    void ReGenerateMap();

    void NetworkUpdate(double _dt);
    void OnLogin();
    void OnDisconnect(SessionStatus disconnectStatus);

    void ImGUI_BroadcastMessageWindow();
    void ImGUI_PlayerInfoWindow();
    void ImGUI_LoginStatusWindow();
    void ImGUI_LoginWindow();
    void ImGUI_ChampionSelectWindow();
    void ImGUI_DrawChatWindow();
    void ImGUI_DebugWindow();

    void scroll_callback(GLFWwindow* window, double xpos, double ypos);
    void cursor_callback(GLFWwindow* window, double xpos, double ypos);
    void mouse_callback(GLFWwindow* window, int button, int action, int mods);
    void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
    void char_callback(GLFWwindow* window, unsigned int codepoint) const;

};
