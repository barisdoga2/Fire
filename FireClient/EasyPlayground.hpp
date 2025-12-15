#pragma once

#include "EasyNet.hpp"
#include "ClientNetwork.hpp"



class EasyDisplay;
class EasyModel;
class EasyShader;
class EasyCamera;
class HDR;
class Chunk;
class EasyBufferManager;
class EasyEntity;
struct GLFWwindow;

class EasyPlayground : public ClientCallback {
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

    // Shaders
    EasyShader* shader{};
    EasyShader* normalLinesShader{};
    EasyCamera* camera{};

    // Assets
    EasyModel* model{};
    EasyModel* cube_1x1x1{};
    EasyModel* items{};
    EasyModel* buildings{};
    EasyModel* walls{};
    std::vector<Chunk*> chunks{};
    HDR* hdr{};

    // Collections
    EasyEntity* player{};
    std::vector<EasyEntity*> players{};

    // ImGUI
    bool mb1_pressed{};
    bool imgui_triangles{};
    bool imgui_isFog{};
    bool imgui_showNormalLines{};
    bool isRender{};
    bool isTestRender{};
    bool rememberMe{};
    float imgui_showNormalLength = 2.2f;

    // Etc
    int animation = 1;
    int seed = 1337;

    EasyPlayground(EasyDisplay* display, EasyBufferManager* bm);
    ~EasyPlayground();

    bool Init();

    bool Update(double _dt);
    bool Render(double _dt);
    void StartRender(double _dt);
    void EndRender();
    void ReloadShaders();
    void ReloadAssets();
    void ReGenerateMap();

    void ProcesPlayer(const std::vector<sPlayerInfo>& infos, const std::vector<sPlayerState>& states);
    void NetworkUpdate(double _dt);
    void OnLogin() override;
    void OnDisconnect() override;

    void ImGUI_Init();
    void ImGUI_Render();
    void ImGUI_TestRender();
    void ImGUI_DrawConsoleWindow();
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

    static void ForwardStandartIO();

};
