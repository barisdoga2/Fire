#pragma once

#include <vector>
#include <string>
#include <EasyIO.hpp>

class EasyBufferManager;
class FireServer;
class EasyDisplay;
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
    EasyDisplay* display;
    FireServer* server;
    double fps = 0.0, ups = 0.0;

    ServerUI(EasyDisplay* display, EasyBufferManager* bm, FireServer* server);
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

    bool mouse_callback(const MouseData& md) override;
    bool scroll_callback(const MouseData& md) override;
    bool cursorMove_callback(const MouseData& md) override;
    bool key_callback(const KeyboardData& data) override;
    bool character_callback(const KeyboardData& data) override;

    static void ForwardStandartIO();

};
