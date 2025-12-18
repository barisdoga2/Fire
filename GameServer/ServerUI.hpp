#pragma once

#include <vector>
#include <string>
#include <EasyIO.hpp>

class EasyBufferManager;
class GameServer;
struct GLFWwindow;
class ServerUI : public MouseListener, KeyboardListener {
private:
    enum CommandType {
        CONSOLE_COMMAND,
        BROADCAST_COMMAND,
        SHUTDOWN_COMMAND,
        START_COMMAND
    };

public:
    const double snapshotPeriod = 2.5;

    EasyBufferManager* bm;
    GameServer* server;
    double fps = 0.0, ups = 0.0;

    ServerUI(EasyBufferManager* bm, GameServer* server);
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

    bool button_callback(const MouseData& data) override;
    bool scroll_callback(const MouseData& data) override;
    bool move_callback(const MouseData& data) override;
    bool key_callback(const KeyboardData& data) override;
    bool char_callback(const KeyboardData& data) override;

    static void ForwardStandartIO();

};
