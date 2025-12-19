#pragma once

#include <unordered_map>

#include <glm/glm.hpp>
#include "EasyIO.hpp"

// ###########
// BASE CAMERA
class EasyEntity;
struct GLFWwindow;
class EasyCamera : public MouseListener, public KeyboardListener {
protected:
	const glm::vec3 worldUp;
	const float far, near, fov;

	glm::vec3 position;
	glm::vec3 rotation;

	glm::vec3 front;
	glm::vec3 up;
	glm::vec3 right;

	glm::mat4 viewMatrix;
	glm::mat4 projectionMatrix;

public:
	bool enabled{true};

	EasyCamera(glm::vec3 position = glm::vec3(0, 0, 0), glm::vec3 rotation = glm::vec3(0, 0, 0), float far = 10000.f, float near = 0.01f, float fov = 90.f, glm::vec3 worldUp = glm::vec3(0.f, 1.f, 0.f));

	float Far() const { return far; };
	float Near() const { return near; };
	float Fov() const { return fov; };
	glm::mat4x4 ViewMatrix() const { return viewMatrix; };
	glm::mat4x4 ProjectionMatrix() const { return projectionMatrix; };
	glm::vec3 Position() const { return position; };
	glm::vec3 Rotation() const { return rotation; };
	glm::vec3 Front() const { return front; };
	glm::vec3 Up() const { return up; };
	glm::vec3 Right() const { return right; };

	virtual void Update(double dt) = 00U;

protected:
	void UpdateVectors();
	void UpdateMatrices(glm::vec3 targetPosition);

};

// ###################
// THIRD PERSON CAMERA
class TPCamera : public EasyCamera {
	float distance = 6.0f;
	float minDist = 0.0f;
	float maxDist = 15.0f;

	glm::vec3 eyeOffset;

	float mouseSensitivity = 0.15f;
	float scrollSensitivity = 1.0f;

	bool rotating = false;
	bool firstMouse = true;
	glm::dvec2 lastMouse{};

	EasyEntity* target;

public:
	TPCamera(EasyEntity* target, glm::vec3 eyeOffset = glm::vec3(0,0,0), float far = 10000.f, float near = 0.01f, float fov = 90.f);

	bool Rotating() const { return rotating; };

	void Update(double dt) override;

	bool button_callback(const MouseData& data) override;
	bool scroll_callback(const MouseData& data) override;
	bool move_callback(const MouseData& data) override;

	bool key_callback(const KeyboardData& data) override;
	bool char_callback(const KeyboardData& data) override;
};


// FREE ROAM CAMERA
class FRCamera : public EasyCamera {
	bool frRotating = false;
	bool frFirstMouse = true;
	glm::dvec2 frLastMouse{};

	float scroll{};
	float frMoveSpeed = 8.0f;
	float frMouseSensitivity = 0.12f;
	float frScrollSensitivity = 50.0f;

	bool keyState[MAX_KEYS]{};

public:
	FRCamera(
		glm::vec3 position = glm::vec3(0),
		glm::vec3 rotation = glm::vec3(0),
		float far = 10000.f,
		float near = 0.01f,
		float fov = 90.f);

	void Update(double dt) override;

	bool button_callback(const MouseData& data) override;
	bool scroll_callback(const MouseData& data) override;
	bool move_callback(const MouseData& data) override;
	bool key_callback(const KeyboardData& data) override;
	bool char_callback(const KeyboardData& data) override;
};
