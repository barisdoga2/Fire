#pragma once

#include <EasyNet.hpp>
#include <EasyIO.hpp>
#include <EasyRender.hpp>
#include <EasyEntity.hpp>
#include <EasyConsole.hpp>
#include "ClientNetwork.hpp"
#include "Player.hpp"

#define MAIN_CHARACTER "MainCharacter"
//#define FIRE_LIGHTWEIGHT

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
    EasyConsole console{};

    // Network
    EasyBufferManager* bm{};
    ClientNetwork* network{};

    // Shaders
    RenderData renderData{};

    // Assets and Collections
    HDR* hdr{};
    std::unordered_map<std::string, EasyModel*> models{};
    EasyModel*& Model(std::string name) { return models[name]; };
    std::unordered_map<std::string, EasyEntity*> entities{};
    std::vector<Chunk*> chunks{};
    MainPlayer* player{};
    std::vector<Player*> players{};
    EasyCamera* camera{};

    // ImGUI
    bool isRender{};
    bool isTestRender{};
    bool rememberMe{};

    // Etc
    int animation = 1;
    int seed = 1337;

    EasyPlayground();
    ~EasyPlayground();

    // Update
    bool Update(double _dt);
    void NetworkUpdate(double _dt);

    // Render
    void ImGUI_Render(double _dt);
    void RenderScene(double _dt);
    bool Render(double _dt);
    void StartRender(double _dt);
    void EndRender(double _dt);

    // Loading
    bool Init(EasyBufferManager* bm);
    void DeInit();
    void ReloadShaders();
    void ReloadAssets();
    void ReGenerateMap();

    // Network
    void ProcessReceived(double _dt);
    void OnLogin() override;
    void OnDisconnect() override;
    void ClearPlayers();
    void CreateMainPlayer(ClientNetwork* network, UserID_t uid = 0U);
    void ProcesPlayer(const std::vector<sPlayerInfo>& infos, const std::vector<sPlayerState>& states);

    // Callbacks
    bool button_callback(const MouseData& data) override;
    bool scroll_callback(const MouseData& data) override;
    bool move_callback(const MouseData& data) override;
    bool key_callback(const KeyboardData& data) override;
    bool char_callback(const KeyboardData& data) override;

    // ImGUI
    void ImGUI_Init();
    void ImGUI_TestRender();
    void ImGUI_DrawConsoleWindow();
    void ImGUI_BroadcastMessageWindow();
    void ImGUI_PlayerInfoWindow();
    void ImGUI_LoginStatusWindow();
    void ImGUI_LoginWindow();
    void ImGUI_ChampionSelectWindow();
    void ImGUI_DrawChatWindow();
    void ImGUI_DebugWindow();

    // Utility
    bool ShortcutManagement(const KeyboardData& data);

    Player* GetPlayerByUID(UserID_t uid);

};

