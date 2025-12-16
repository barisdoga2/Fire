#pragma once

#include <EasyNet.hpp>
#include <EasyIO.hpp>
#include <EasyRender.hpp>
#include <EasyEntity.hpp>
#include "ClientNetwork.hpp"
#include "Player.hpp"



class EasyDisplay;
class EasyModel;
class EasyShader;
class EasyCamera;
class HDR;
class Chunk;
class EasyBufferManager;
class EasyEntity;
struct GLFWwindow;
class EasyPlayground : public ClientCallback, KeyboardListener, MouseListener {
private:
public:
    // Display
    EasyDisplay* display{};
    double fps{};
    double ups{};

    // Network
    EasyBufferManager* bm{};
    ClientNetwork* network;

    // Shaders
    RenderData renderData{};
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
    Player* player{};
    std::vector<Player*> players{};

    // ImGUI
    bool isRender{};
    bool isTestRender{};
    bool rememberMe{};

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

    bool mouse_callback(const MouseData& md) override;
    bool scroll_callback(const MouseData& md) override;
    bool cursorMove_callback(const MouseData& md) override;
    bool key_callback(const KeyboardData& data) override;
    bool character_callback(const KeyboardData& data) override;

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

    static void ForwardStandartIO();

    inline Player* GetPlayerByUID(UserID_t uid)
    {
        Player* ret{};
        for (Player* p : players)
        {
            if (p->uid == uid)
            {
                ret = p;
                break;
            }
        }
        return ret;
    }
};
