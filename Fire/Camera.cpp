#include "Camera.hpp"
#include "Utils.hpp"

Camera::Camera(glm::vec3 position, glm::vec3 rotation, float fov, glm::tvec2<int> windowSize, float nearPlane, float farPlane)
{
	this->position = position;
	this->rotation = rotation;
	this->fov = fov;
	this->windowSize = windowSize;
	this->aspectRatio = windowSize.x / (float)windowSize.y;
	this->nearPlane = nearPlane;
	this->farPlane = farPlane;

	BuildMatrices();
}

void Camera::Update(float deltaMs)
{
	glm::vec3 amount = scrollDistance * (freeRoomSpeedMetersPerSec / 1000.0f) * deltaMs;
	scrollDistance -= amount;

	position -= amount;

	BuildMatrices();
}

void Camera::BuildMatrices()
{
	// View Matrix
	glm::mat4x4 viewMatrix = glm::mat4x4(1);
	viewMatrix = glm::rotate(viewMatrix, (float)glm::radians(rotation.x), glm::vec3(1, 0, 0));
	viewMatrix = glm::rotate(viewMatrix, (float)glm::radians(rotation.y), glm::vec3(0, 1, 0));
	viewMatrix = glm::translate(viewMatrix, -position);
	this->viewMatrix = viewMatrix;

	// Projection Matrix
	this->projectionMatrix = glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);

	// Create Vectors
	glm::mat4x4 um = glm::mat4x4(1);
	um = glm::rotate(um, glm::radians(rotation.x), glm::vec3(-1, 0, 0));
	um = glm::translate(um, globalUp);
	this->up = glm::vec3(um[3][0], um[3][1], um[3][2]);

	glm::mat4x4 fm = glm::mat4x4(1);
	fm = glm::rotate(fm, glm::radians(rotation.y), glm::vec3(0, -1, 0));
	fm = glm::rotate(fm, glm::radians(rotation.x), glm::vec3(-1, 0, 0));
	fm = glm::translate(fm, glm::vec3(0, 0, -1));
	this->front = glm::vec3(fm[3][0], fm[3][1], fm[3][2]);

	glm::mat4x4 lm = glm::mat4x4(1);
	lm = glm::rotate(lm, glm::radians(rotation.y), glm::vec3(0, -1, 0));
	lm = glm::translate(lm, glm::vec3(-1, 0, 0));
	this->left = glm::vec3(lm[3][0], lm[3][1], lm[3][2]);
}

bool Camera::mouse_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
	{
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
		rotating = true;
		return true;
	}
	else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
	{
		glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		rotating = false;
		return true;
	}
	return false;
}

bool Camera::scroll_callback(GLFWwindow* window, double x, double y)
{
	glm::dvec2 mouseScreenPosition = ToScreenCoords(Mouse::data.position.now, windowSize);
	glm::vec3 mouseRay = CreateRay(viewMatrix, projectionMatrix, mouseScreenPosition);
	scrollDistance += mouseRay * scrollSensitivity * (float)-y;
	return true;
}

bool Camera::cursorMove_callback(glm::vec2 normalizedDelta, bool checkCursor)
{
	if (rotating)
	{
		float rotationalSpeed = 0.1f;
		glm::vec2 delta = glm::vec2(normalizedDelta.x * rotationalSpeed, normalizedDelta.y * rotationalSpeed); // Sensitivity
		rotation.y += delta.x;
		rotation.x += delta.y * aspectRatio;

		rotation.x = fmod(rotation.x, 360.0f);
		rotation.y = fmod(rotation.y, 360.0f);

		return true;
	}

	return false;
}

bool Camera::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	return false;
}


bool Camera::character_callback(GLFWwindow* window, unsigned int codepoint)
{
	return false;
}