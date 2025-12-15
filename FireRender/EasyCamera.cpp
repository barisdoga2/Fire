#include "pch.h"
#include "EasyCamera.hpp"
#include "EasyDisplay.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

EasyCamera::EasyCamera(EasyDisplay* display, glm::vec3 pos, glm::vec3 target, float fovP, float nearP, float farP)
    : display(display)
{
    fov = fovP;
    far = farP;
    near = nearP;
    position = pos;
    front = glm::normalize(target - pos);
    aspect = display->windowSize.x / (float)display->windowSize.y;
    UpdateVectors();

    targetPos = position;
    targetFront = front;

    projection_ = glm::perspective(glm::radians(fov), display->windowSize.x / (float)display->windowSize.y, nearP, farP);
    view_ = glm::lookAt(position, position + front, up);

    // Default: third person follow camera. Legacy free-fly can be enabled via followMode=false.
    followYaw = yaw;
    followPitch = pitch;
    ModeSwap(false);
}

void EasyCamera::ModeSwap(bool mode)
{
    // In follow mode, RMB hold is handled in mouse_callback; don't toggle here.
    if (followMode)
    {
        this->mode = false;
        glfwSetInputMode(display->window, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
        glfwSetInputMode(display->window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        return;
    }

    if (this->mode == mode)
        return;

    this->mode = mode;
    if (mode)
    {
        firstMouse = true;
        mouseDelta = glm::vec2(0.0f);
        glfwSetInputMode(display->window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        glfwSetInputMode(display->window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }
    else
    {
        glfwSetInputMode(display->window, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
        glfwSetInputMode(display->window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
}

bool EasyCamera::mouse_callback(const MouseData& md)
{
    // Follow mode: RMB hold to rotate camera (WoW-like)
    if (followMode && md.button.button == GLFW_MOUSE_BUTTON_2)
    {
        if (md.button.action == GLFW_PRESS)
        {
            rmbLook = true;
            firstMouse = true;
            mouseDelta = glm::vec2(0.0f);
            glfwSetInputMode(display->window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            glfwSetInputMode(display->window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
        }
        else if (md.button.action == GLFW_RELEASE)
        {
            rmbLook = false;
            glfwSetInputMode(display->window, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
            glfwSetInputMode(display->window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }
    else if (!followMode)
    {
        // Legacy: toggle free-fly mode with RMB release
        if (md.button.button == GLFW_MOUSE_BUTTON_2 && md.button.action == GLFW_RELEASE)
        {
            ModeSwap(!mode);
        }
    }

    return false;
}

bool EasyCamera::scroll_callback(const MouseData& md)
{
    if (followMode)
    {
        followDistance -= (float)md.scroll.now.y * followZoomSpeed;
        followDistance = glm::clamp(followDistance, followMinDistance, followMaxDistance);
        return false;
    }

    scrollDelta = (float)md.scroll.now.y * moveSpeed * 15;
    return false;
}

bool EasyCamera::cursorMove_callback(const MouseData& md)
{
    if (followMode)
    {
        if (!rmbLook)
            return false;
    }
    else
    {
        if (!mode)
            return false;
    }

    if (firstMouse) {
        lastX = md.position.now.x;
        lastY = md.position.now.y;
        firstMouse = false;
    }

    mouseDelta.x += (float)(md.position.now.x - lastX);
    mouseDelta.y += (float)(lastY - md.position.now.y);
    lastX = md.position.now.x;
    lastY = md.position.now.y;

    return false;
}

bool EasyCamera::key_callback(const KeyboardData& data)
{
    if (followMode || !mode)
        return false;

    if (data.action == GLFW_PRESS)
        keyState[data.key] = true;
    else if (data.action == GLFW_RELEASE)
        keyState[data.key] = false;
    else
        return false;

    if (data.key == GLFW_KEY_W || data.key == GLFW_KEY_S || data.key == GLFW_KEY_A || data.key == GLFW_KEY_D || data.key == GLFW_KEY_SPACE || data.key == GLFW_KEY_LEFT_SHIFT)
        return true;
    else
        return false;

    return false;
}

bool EasyCamera::character_callback(const KeyboardData& data)
{
    return false;
}

void EasyCamera::Update(double dt)
{
    if (followMode)
    {
        // 1) Rotation from RMB mouse look
        float dx = mouseDelta.x * mouseSensitivity;
        float dy = mouseDelta.y * mouseSensitivity;
        mouseDelta = glm::vec2(0.0f);

        followYaw += dx;
        followPitch += dy;
        followPitch = glm::clamp(followPitch, followPitchMin, followPitchMax);

        // 2) Compute desired camera position/orientation around target
        const glm::vec3 target = followTarget + glm::vec3(0.f, followHeight, 0.f);
        glm::vec3 desiredFront;
        desiredFront.x = cos(glm::radians(followYaw)) * cos(glm::radians(followPitch));
        desiredFront.y = sin(glm::radians(followPitch));
        desiredFront.z = sin(glm::radians(followYaw)) * cos(glm::radians(followPitch));
        desiredFront = glm::normalize(desiredFront);

        const glm::vec3 desiredPos = target - desiredFront * followDistance;

        // 3) Smooth
        position = glm::mix(position, desiredPos, (float)glm::clamp(dt * followPosLerp, 0.0, 1.0));
        front = glm::normalize(glm::mix(front, desiredFront, (float)glm::clamp(dt * followRotLerp, 0.0, 1.0)));
        UpdateVectors();
        view_ = glm::lookAt(position, position + front, up);
        return;
    }

    if (!mode)
        return;

    // Legacy free-fly:
    float dx = mouseDelta.x * mouseSensitivity;
    float dy = mouseDelta.y * mouseSensitivity;
    mouseDelta = glm::vec2(0.0f);

    yaw += dx;
    pitch += dy;
    pitch = glm::clamp(pitch, -89.0f, 89.0f);

    glm::vec3 newFront;
    newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    newFront.y = sin(glm::radians(pitch));
    newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    targetFront = glm::normalize(newFront);

    UpdateVectors();

    glm::vec3 move(0.f);
    if (keyState[GLFW_KEY_W]) move += front;
    if (keyState[GLFW_KEY_S]) move -= front;
    if (keyState[GLFW_KEY_A]) move -= right;
    if (keyState[GLFW_KEY_D]) move += right;
    if (keyState[GLFW_KEY_SPACE]) move += up;
    if (keyState[GLFW_KEY_LEFT_SHIFT]) move -= up;

    if (glm::length(move) > 0.0f)
        move = glm::normalize(move);

    move += front * scrollDelta;
    scrollDelta = 0.0f;

    targetPos += move * moveSpeed * (float)dt;

    position = glm::mix(position, targetPos, (float)(dt * lerpSpeed));
    front = glm::normalize(glm::mix(front, targetFront, (float)(dt * lerpSpeed)));

    UpdateVectors();
    view_ = glm::lookAt(position, position + front, up);
}

glm::vec3 EasyCamera::GetFlatForward() const
{
    glm::vec3 f = front;
    f.y = 0.0f;
    const float len = glm::length(f);
    return (len > 0.0001f) ? (f / len) : glm::vec3(0.f, 0.f, -1.f);
}

float EasyCamera::GetYawDeg() const
{
    return followMode ? followYaw : yaw;
}

void EasyCamera::UpdateVectors()
{
    right = glm::normalize(glm::cross(front, worldUp));
    up = glm::normalize(glm::cross(right, front));
}
