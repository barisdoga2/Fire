#include "pch.h"
#include "EasyCamera.hpp"
#include "EasyDisplay.hpp"
#include "EasyEntity.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

// ###########
// BASE CAMERA
EasyCamera::EasyCamera(glm::vec3 position, glm::vec3 rotation, float far, float near, float fov, glm::vec3 worldUp) : position(position), rotation(rotation), far(far), near(near), fov(fov), worldUp(worldUp)
{
    UpdateVectors();
    UpdateMatrices(position + front);
}

void EasyCamera::UpdateMatrices(glm::vec3 targetPosition)
{
    viewMatrix = glm::lookAt(position, targetPosition, up);

    projectionMatrix = glm::perspective(glm::radians(fov), EasyDisplay::GetAspectRatio(), near, far);
}

void EasyCamera::UpdateVectors()
{
    rotation.x = glm::clamp(rotation.x, -89.0f, 89.0f);

    glm::vec3 f;
    f.x = cos(glm::radians(rotation.y)) * cos(glm::radians(rotation.x));
    f.y = sin(glm::radians(rotation.x));
    f.z = sin(glm::radians(rotation.y)) * cos(glm::radians(rotation.x));

    front = glm::normalize(f);
    right = glm::normalize(glm::cross(front, worldUp));
    up = glm::normalize(glm::cross(right, front));
}

// ###################
// THIRD PERSON CAMERA
TPCamera::TPCamera(EasyEntity* target, glm::vec3 eyeOffset, float far, float near, float fov) 
    : target(target), eyeOffset(eyeOffset), EasyCamera({ 0.1f, 2.9f, 2.7f }, { -23.f, 90.f, 0.f }, far, near, fov)
{
    
}

void TPCamera::Update(double dt)
{
    if (!enabled)
        return;

    if (!target)
        return;

    UpdateVectors();

    const glm::vec3 targetPos = target->transform.position + eyeOffset;

    position = targetPos - front * distance;

    UpdateMatrices(targetPos);
}

bool TPCamera::button_callback(const MouseData& data)
{
    if (!enabled)
        return false;

    if (data.button.button == GLFW_MOUSE_BUTTON_2 || data.button.button == GLFW_MOUSE_BUTTON_1)
    {
        rotating = (data.button.action == GLFW_PRESS);
        firstMouse = true;
        EasyMouse::EnableCursor(!rotating);
        return true;
    }		
    return false;
}

bool TPCamera::scroll_callback(const MouseData& data)
{
    if (!enabled)
        return false;

    distance -= (float)data.scroll.now.y * scrollSensitivity;
    distance = glm::clamp(distance, minDist, maxDist);
    return true;
}

bool TPCamera::move_callback(const MouseData& data)
{
    if (!enabled)
        return false;

    if (!rotating)
        return false;

    if (firstMouse)
    {
        lastMouse = data.position.now;
        firstMouse = false;
        return true;
    }

    glm::vec2 delta = data.position.now - lastMouse;
    lastMouse = data.position.now;

    rotation.y += delta.x * mouseSensitivity; 
    rotation.x -= delta.y * mouseSensitivity; 

    return true;
}

bool TPCamera::key_callback(const KeyboardData& data) 
{ 
    if (!enabled)
        return false;

    return false; 
}

bool TPCamera::char_callback(const KeyboardData& data) 
{ 
    if (!enabled)
        return false;

    return false;
}

// ################
// FREE ROAM CAMERA
FRCamera::FRCamera(glm::vec3 position, glm::vec3 rotation, float far, float near, float fov) : EasyCamera(position, rotation, far, near, fov)
{

}

void FRCamera::Update(double dt)
{
    if (!enabled)
        return;

    UpdateVectors();

    float speed = frMoveSpeed * static_cast<float>(dt);

    if (keyState[GLFW_KEY_W]) position += front * speed;
    if (keyState[GLFW_KEY_S]) position -= front * speed;
    if (keyState[GLFW_KEY_A]) position -= right * speed;
    if (keyState[GLFW_KEY_D]) position += right * speed;

    if (keyState[GLFW_KEY_SPACE])       position += worldUp * speed;
    if (keyState[GLFW_KEY_LEFT_SHIFT])  position -= worldUp * speed;

    position += front * scroll * speed;
    scroll = scroll * 0.90f;

    UpdateMatrices(position + front);
}


bool FRCamera::move_callback(const MouseData& data)
{
    if (!enabled)
        return false;

    if (!frRotating)
        return false;

    if (frFirstMouse)
    {
        frLastMouse = data.position.now;
        frFirstMouse = false;
        return true;
    }

    glm::vec2 delta = data.position.now - frLastMouse;
    frLastMouse = data.position.now;

    rotation.y += delta.x * frMouseSensitivity;
    rotation.x -= delta.y * frMouseSensitivity;

    return true;
}

bool FRCamera::scroll_callback(const MouseData& data)
{
    if (!enabled)
        return false;

    scroll = (float)data.scroll.now.y;
    return true;
}

bool FRCamera::button_callback(const MouseData& data)
{
    if (!enabled)
        return false;

    if (data.button.button == GLFW_MOUSE_BUTTON_2)
    {
        frRotating = (data.button.action == GLFW_PRESS);
        frFirstMouse = true;
        return true;
    }
    return false;
}

bool FRCamera::key_callback(const KeyboardData& data)
{
    if (!enabled)
        return false;

    if (data.key < 0 || data.key > GLFW_KEY_LAST)
        return false;

    if (data.action == GLFW_PRESS)
        keyState[data.key] = true;
    else if (data.action == GLFW_RELEASE)
        keyState[data.key] = false;

    return true; // consume
}


bool FRCamera::char_callback(const KeyboardData& data)
{
    if (!enabled)
        return false;

    return false;
}
