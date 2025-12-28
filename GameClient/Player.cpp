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

bool Player::Update(double _dt) 
{
	if (stateManager)
		stateManager->Update(_dt);
	
	if(animator)
		animator->UpdateAnimation(_dt);

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
