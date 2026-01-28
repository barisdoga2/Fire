#include "Player.hpp"
#include <EasyIO.hpp>
#include <EasyModel.hpp>
#include <EasyAnimator.hpp>
#include <EasyAnimation.hpp>
#include <EasyCamera.hpp>

#include <GLFW/glfw3.h>
#include <glm/gtx/quaternion.hpp>

#include "ClientNetwork.hpp"
#include "AnimationSM_unity.hpp"

Player::Player(ClientNetwork* network, EasyModel* model, UserID_t uid, bool isMainPlayer, glm::vec3 position) 
	: network(network), uid(uid), isMainPlayer(isMainPlayer), EasyEntity(model, position)
{
	
}

Player::~Player()
{
	if (stateManager)
		delete stateManager;
}

void Player::AssetReadyCallback()
{
	EasyEntity::AssetReadyCallback();
	if(animator)
		stateManager = new AnimationSM(animator);
}

void Player::ApplyServerState(const sPlayerState& state)
{
    prevPosition = transform.position;
    prevRotation = transform.rotationQuat;
    serverPosition = state.position;
    serverRotation = state.rotation;
    interpTimer = 0.0f;
}

bool Player::Update(double _dt) 
{
    // Interpolate for remote players (not main player)
    if (!isMainPlayer)
    {
        interpTimer += (float)_dt;
        float alpha = glm::clamp(interpTimer / INTERP_DURATION, 0.0f, 1.0f);
        
        transform.position = glm::mix(prevPosition, serverPosition, alpha);
        transform.rotationQuat = glm::slerp(prevRotation, serverRotation, alpha);
    }

    if (stateManager)
        stateManager->Update((float)_dt);
	
    if(animator)
        animator->Update((float)_dt);

    return EasyEntity::Update(_dt);
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
