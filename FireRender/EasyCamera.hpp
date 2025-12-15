#pragma once

#include <unordered_map>

#include <glm/glm.hpp>
#include "EasyIO.hpp"



class EasyDisplay;
struct GLFWwindow;
class EasyCamera : public MouseListener, KeyboardListener {
public:
	EasyDisplay* display;

	glm::vec3 position{ 0.f, 0.f, 3.f };
	glm::vec3 front{ 0.f, 0.f, -1.f };
	glm::vec3 up{ 0.f, 1.f, 0.f };
	glm::vec3 right{ 1.f, 0.f, 0.f };
	glm::vec3 worldUp{ 0.f, 1.f, 0.f };
	bool mode = false;

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
