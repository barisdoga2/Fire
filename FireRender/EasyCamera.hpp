#pragma once

#include <unordered_map>

#include <glm/glm.hpp>
#include "EasyIO.hpp"

class EasyDisplay;
struct GLFWwindow;

class EasyCamera : public MouseListener, KeyboardListener {
public:
    EasyDisplay* display;

    // --- Third person (WoW-like) follow camera ---
    bool followMode = true;                 // if true, camera orbits a target
    glm::vec3 followTarget{ 0.f };          // world-space follow point (typically player's position)
    float followDistance = 6.0f;            // behind target
    float followHeight = 1.6f;              // target offset (eye height)
    float followMinDistance = 2.0f;
    float followMaxDistance = 18.0f;
    float followZoomSpeed = 2.5f;           // scroll zoom
    float followYaw = -90.0f;               // degrees
    float followPitch = -18.0f;             // degrees (negative looks down)
    float followPitchMin = -75.0f;
    float followPitchMax = 25.0f;
    float followPosLerp = 18.0f;
    float followRotLerp = 22.0f;
    bool rmbLook = false;                   // holding RMB rotates camera

    // Helper: set the point the camera follows (call every frame with player position)
    void SetFollowTarget(const glm::vec3& worldPos) { followTarget = worldPos; }
    glm::vec3 GetFlatForward() const;       // normalized, y=0 forward
    float GetYawDeg() const;                // yaw used for flat forward

    glm::vec3 position{ 0.f, 0.f, 3.f };
    glm::vec3 front{ 0.f, 0.f, -1.f };
    glm::vec3 up{ 0.f, 1.f, 0.f };
    glm::vec3 right{ 1.f, 0.f, 0.f };
    glm::vec3 worldUp{ 0.f, 1.f, 0.f };

    bool mode = false; // legacy free-fly mode (toggle with RMB in non-follow mode)

    float aspect = 0.f;
    float fov = 90.f;
    float yaw = -89.f;
    float pitch = -26.f;

    float far = 0.f, near = 0.f;

    glm::mat4 view_{ 1.f };
    glm::mat4 projection_{ 1.f };

    float moveSpeed = 5.f;
    float mouseSensitivity = 0.13f;
    float lerpSpeed = 10.f;

    // --- Input state ---
    glm::vec2 mouseDelta{ 0.f };
    float scrollDelta = 0.f;
    std::unordered_map<int, bool> keyState;

    bool firstMouse = true;
    double lastX = 0.0, lastY = 0.0;

    // --- Target states for smoothing ---
    glm::vec3 targetPos;
    glm::vec3 targetFront;

    EasyCamera(EasyDisplay* display, glm::vec3 pos, glm::vec3 target, float fov, float nearP, float farP);
    void Update(double dt);
    void ModeSwap(bool mode = false);

    bool mouse_callback(const MouseData& md) override;
    bool scroll_callback(const MouseData& md) override;
    bool cursorMove_callback(const MouseData& md) override;
    bool key_callback(const KeyboardData& data) override;
    bool character_callback(const KeyboardData& data) override;

private:
    void UpdateVectors();
};
