#pragma once

#include "EasyCamera.hpp"
#include "EasyShader.hpp"
#include "EasyModel.hpp"
#include "EasyAnimator.hpp"
#include "EasyDisplay.hpp"
#include "ChunkRenderer.hpp"
#include "SkyboxRenderer.hpp"


class HDR;
class EasyPlayground {
private:
public:
    int exitRequested = 0;
    int animation = 1;
    bool mb1_pressed = false;
    const EasyDisplay& display_;
    EasyModel* model = EasyModel::LoadModel(GetPath("res/models/Kachujin G Rosales Skin.fbx"), { GetPath("res/models/Standing Idle on Kachujin G Rosales wo Skin.fbx"), GetPath("res/models/Running on Kachujin G Rosales wo Skin.fbx"), GetPath("res/models/Standing Aim Idle 01 on Kachujin H Rosales wo Skin.fbx") });
    EasyModel* cube_1x1x1 = EasyModel::LoadModel(GetPath("res/models/1x1cube.dae"));
    EasyModel* items = EasyModel::LoadModel(GetPath("res/models/items.dae"));
    EasyModel* buildings = EasyModel::LoadModel(GetPath("res/models/Buildings.dae"));
    EasyModel* walls = EasyModel::LoadModel(GetPath("res/models/Walls.dae"));
    
    EasyShader shader = EasyShader("model");
    EasyShader normalLinesShader = EasyShader("NormalLines");
    EasyCamera camera = EasyCamera(display_, { 1,4.4,5.8 }, { 1 - 0.15, 4.4 - 0.44,5.8 - 0.895 }, 74.f, 0.01f, 1000.f);
    bool imgui_triangles{};
    bool imgui_isFog{};
    bool imgui_showNormalLines{};
    float imgui_showNormalLength = 2.2f;
    double fps = 0.0, ups = 0.0;
    int seed = 1337;

    HDR* hdr;
    std::vector<Chunk*> chunks;

    std::vector<EasyModel*> mapObjects;

    EasyPlayground(const EasyDisplay& display);
    ~EasyPlayground();

    bool Init();

    bool Update(double _dt);
    bool Render(double _dt);
    void StartRender(double _dt);
    void EndRender();
    void ImGUIRender();
    void ReloadShaders();
    void ReGenerateMap();

    void ImGUI_BroadcastMessageWindow();
    void ImGUI_PlayerInfoWindow();
    void ImGUI_LoginStatusWindow();
    void ImGUI_LoginWindow();
    void ImGUI_ChampionSelectWindow();

    void scroll_callback(GLFWwindow* window, double xpos, double ypos);
    void cursor_callback(GLFWwindow* window, double xpos, double ypos);
    void mouse_callback(GLFWwindow* window, int button, int action, int mods);
    void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
    void char_callback(GLFWwindow* window, unsigned int codepoint);

};
