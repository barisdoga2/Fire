#include "Player.hpp"
#include <EasyModel.hpp>
#include <EasyAnimator.hpp>
#include <EasyAnimation.hpp>
#include "ClientNetwork.hpp"




Player::Player(ClientNetwork* network, EasyModel* model, UserID_t uid, bool isMainPlayer, glm::vec3 position) : network(network), uid(uid), isMainPlayer(isMainPlayer), EasyEntity(model, position)
{

}

Player::~Player()
{

}

bool Player::Update(double _dt) 
{
    if (!animator && model->isRawDataLoadedToGPU && model->animations.size() > 0u)
        animator = new EasyAnimator(model->animations.at(0));
    else if (animator)
        animator->UpdateAnimation(_dt);

    if (!isMainPlayer)
        return true;

    return true;
}

bool Player::mouse_callback(const MouseData& md)
{
	
	return false;
}

bool Player::scroll_callback(const MouseData& md)
{
	
	return false;
}

bool Player::cursorMove_callback(const MouseData& md)
{
	
	return false;
}

bool Player::key_callback(const KeyboardData& data)
{
	
	return false;
}

bool Player::character_callback(const KeyboardData& data)
{
	
	return false;
}
