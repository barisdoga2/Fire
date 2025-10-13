#pragma once

#include <unordered_map>

#include <glm/glm.hpp>



class EasyDisplay;
struct GLFWwindow;
class EasyCamera {
public:
	const EasyDisplay& display;

	glm::vec3 position{ 0.f, 0.f, 3.f };
	glm::vec3 front{ 0.f, 0.f, -1.f };
	glm::vec3 up{ 0.f, 1.f, 0.f };
	glm::vec3 right{ 1.f, 0.f, 0.f };
	glm::vec3 worldUp{ 0.f, 1.f, 0.f };
	bool mode = true;

	float yaw = -89.f;
	float pitch = -26.f;

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

    EasyCamera(const EasyDisplay& display, glm::vec3 pos, glm::vec3 target, float fov, float nearP, float farP);
    void Update(double dt);
    void ModeSwap();

    void cursor_callback(double xpos, double ypos);
    void scroll_callback(double xoffset, double yoffset);
    void key_callback(int key, int scancode, int action, int mods);
    void mouse_callback(GLFWwindow* window, int button, int action, int mods);

private:
    void UpdateVectors();
};
