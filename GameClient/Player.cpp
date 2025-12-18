#include "Player.hpp"
#include <EasyIO.hpp>
#include <EasyModel.hpp>
#include <EasyAnimator.hpp>
#include <EasyAnimation.hpp>
#include "ClientNetwork.hpp"

#include <GLFW/glfw3.h>


Player::Player(ClientNetwork* network, EasyModel* model, UserID_t uid, bool isMainPlayer, glm::vec3 position) 
: network(network), uid(uid), isMainPlayer(isMainPlayer), targetPosition(position), EasyEntity(model, position)
{
	
}

Player::~Player()
{

}

bool Player::Update(double _dt) 
{
    if (!animator && model->isRawDataLoadedToGPU && model->animations.size() > 0u)
        animator = new EasyAnimator(model->animations.at(0U));

	if (animator)
		animator->UpdateAnimation(_dt);

    return true;
}

bool Player::button_callback(const MouseData& data)
{
	
	return false;
}

bool Player::scroll_callback(const MouseData& data)
{
	
	return false;
}

bool Player::move_callback(const MouseData& data)
{
	
	return false;
}

bool Player::key_callback(const KeyboardData& data)
{
	
	return false;
}

bool Player::char_callback(const KeyboardData& data)
{
	
	return false;
}

MainPlayer::MainPlayer(ClientNetwork* network, EasyModel* model, UserID_t uid, glm::vec3 position) : Player(network, model, uid, true, position)
{

}

MainPlayer::~MainPlayer()
{

}

bool MainPlayer::Update(double _dt)
{
    float dt = (float)_dt;

    bool w = keyStates[GLFW_KEY_W];
    bool s = keyStates[GLFW_KEY_S];
    bool a = keyStates[GLFW_KEY_A];
    bool d = keyStates[GLFW_KEY_D];
    bool rotateOnly = (!w && !s && (a || d));
    bool moveInput = (w || s || a || d);

    float yawRad = glm::radians(transform.rotation.y);
    if (rotateOnly)
    {
        transform.rotation.y -= (a ? -1.f : 1.f) * 180.f * dt;
        transform.rotation.y = glm::mod(transform.rotation.y, 360.f);
        direction = glm::vec3(0.f);
    }
    else
    {
        if (!moveInput)
        {
            direction = glm::mix(direction, glm::vec3(0.f), dt * 10.f);
        }
        else
        {
            glm::vec3 inputDir((a ? -1.f : 0.f) + (d ? 1.f : 0.f), 0.f, (w ? 1.f : 0.f) + (s ? -1.f : 0.f) );
            if (glm::length(inputDir) > 0.f)
                inputDir = glm::normalize(inputDir);

            glm::vec3 desiredDir(sin(yawRad) * inputDir.z + cos(yawRad) * inputDir.x, 0.f, cos(yawRad) * inputDir.z - sin(yawRad) * inputDir.x);
            direction = glm::normalize(glm::mix(direction, desiredDir, dt * 8.f));
        }

        bool positionLerp = false;
        targetPosition += direction * speed * dt;
        transform.position = positionLerp ? ((targetPosition - transform.position) * 0.1f) : targetPosition;

        if (glm::length(direction) > 0.001f)
            transform.rotation.y = glm::degrees(std::atan2(direction.x, direction.z));
    }

    if (animator)
        animator->PlayAnimation(model->animations.at((w || s) ? 1U : 0U));

    return Player::Update(_dt);
}

bool MainPlayer::key_callback(const KeyboardData& data)
{
	if (Player::key_callback(data))
		return true;
	
	if (data.key == GLFW_KEY_W || data.key == GLFW_KEY_A || data.key == GLFW_KEY_S || data.key == GLFW_KEY_D)
	{
		if (data.action == GLFW_PRESS || data.action == GLFW_REPEAT)
		{
			keyStates[data.key] = true;
		}
		else if (data.action == GLFW_RELEASE)
		{
			keyStates[data.key] = false;
		}
		return true;
	}
	return false;
}