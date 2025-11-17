#include "EasyCamera.hpp"
#include "EasyDisplay.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include <GLFW/glfw3.h>



EasyCamera::EasyCamera(const EasyDisplay& display, glm::vec3 pos, glm::vec3 target, float fovP, float nearP, float farP)
    : display(display)
{
    fov = fovP;
    far = farP;
    near = nearP;
    position = pos;
    front = glm::normalize(target - pos);
    aspect = display.windowSize.x / (float)display.windowSize.y;
    UpdateVectors();

    targetPos = position;
    targetFront = front;

    projection_ = glm::perspective(glm::radians(fov), display.windowSize.x / (float)display.windowSize.y, nearP, farP);
    view_ = glm::lookAt(position, position + front, up);

    ModeSwap();
}

void EasyCamera::ModeSwap(bool mode)
{
    if (this->mode == mode)
        return;

    this->mode = mode;
    if (mode)
    {
        firstMouse = true;
        mouseDelta = glm::vec2(0.0f);
        glfwSetInputMode(display.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        glfwSetInputMode(display.window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }
    else
    {
        glfwSetInputMode(display.window, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
        glfwSetInputMode(display.window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
}

void EasyCamera::cursor_callback(double xpos, double ypos)
{
    if (!mode)
        return;
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    mouseDelta.x += (float)(xpos - lastX);
    mouseDelta.y += (float)(lastY - ypos);
    lastX = xpos;
    lastY = ypos;
}

void EasyCamera::scroll_callback(double xoffset, double yoffset)
{
    //if (!mode)
    //    return;
    scrollDelta = (float)yoffset * moveSpeed * 55;
}

bool EasyCamera::key_callback(int key, int scancode, int action, int mods)
{
    if (!mode)
        return false;
    if (action == GLFW_PRESS)
        keyState[key] = true;
    else if (action == GLFW_RELEASE)
        keyState[key] = false;
    else
        return false;

    if (key == GLFW_KEY_W || key == GLFW_KEY_S || key == GLFW_KEY_A || key == GLFW_KEY_D || key == GLFW_KEY_SPACE || key == GLFW_KEY_LEFT_SHIFT)
        return true;
    else
        return false;
}

void EasyCamera::mouse_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_2 && action == GLFW_RELEASE)
    {
        ModeSwap(!mode);
    }
}

void EasyCamera::Update(double dt)
{
    if (!mode)
        return;

    // 1. Handle rotation (from mouse)
    float dx = mouseDelta.x * mouseSensitivity;
    float dy = mouseDelta.y * mouseSensitivity;
    mouseDelta = glm::vec2(0.0f); // reset after use

    yaw += dx;
    pitch += dy;
    pitch = glm::clamp(pitch, -89.0f, 89.0f);

    glm::vec3 newFront;
    newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    newFront.y = sin(glm::radians(pitch));
    newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    targetFront = glm::normalize(newFront);

    UpdateVectors();

    // 2. Handle movement (from WASD)
    glm::vec3 move(0.f);
    if (keyState[GLFW_KEY_W]) move += front;
    if (keyState[GLFW_KEY_S]) move -= front;
    if (keyState[GLFW_KEY_A]) move -= right;
    if (keyState[GLFW_KEY_D]) move += right;
    if (keyState[GLFW_KEY_SPACE]) move += up;
    if (keyState[GLFW_KEY_LEFT_SHIFT]) move -= up;

    if (glm::length(move) > 0.0f)
        move = glm::normalize(move);

    // scroll moves forward/back
    move += front * scrollDelta;
    scrollDelta = 0.0f;

    targetPos += move * moveSpeed * (float)dt;

    // 3. Smooth interpolate position & direction
    position = glm::mix(position, targetPos, (float)(dt * lerpSpeed));
    front = glm::normalize(glm::mix(front, targetFront, (float)(dt * lerpSpeed)));

    UpdateVectors();

    view_ = glm::lookAt(position, position + front, up);
}

void EasyCamera::UpdateVectors()
{
    right = glm::normalize(glm::cross(front, worldUp));
    up = glm::normalize(glm::cross(right, front));
}
