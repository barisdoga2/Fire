#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/quaternion.hpp>
#include <GLFW/glfw3.h>

#include "Mouse.hpp"
#include "Keyboard.hpp"

class Camera : public MouseListener, public KeyboardListener {
public:
	const glm::vec3 globalUp = glm::vec3(0, 1, 0);

	glm::vec3 position;
	glm::vec3 rotation;
	glm::mat4x4 viewMatrix;
	glm::mat4x4 projectionMatrix;
	glm::vec3 up, front, left;
	glm::tvec2<int> windowSize;
	float fov, aspectRatio, nearPlane, farPlane;
	bool rotating = false;

	glm::vec3 scrollDistance = glm::vec3(0.f);
	float scrollSensitivity = 3.f;
	float freeRoomSpeedMetersPerSec = 10.0f;

	Camera(glm::vec3 position, glm::vec3 rotation, float fov, glm::tvec2<int> windowSize, float nearPlane, float farPlane);

	void Update(float deltaMs);

	void BuildMatrices();

	bool mouse_callback(GLFWwindow* window, int button, int action, int mods) override;
	bool scroll_callback(GLFWwindow* window, double x, double y) override;
	bool cursorMove_callback(glm::vec2 normalizedDelta, bool checkCursor = true) override;
	bool key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) override;
	bool character_callback(GLFWwindow* window, unsigned int codepoint) override;
};