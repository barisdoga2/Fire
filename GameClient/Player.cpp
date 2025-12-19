#include "Player.hpp"
#include <EasyIO.hpp>
#include <EasyModel.hpp>
#include <EasyAnimator.hpp>
#include <EasyAnimation.hpp>
#include <EasyCamera.hpp>
#include "ClientNetwork.hpp"

#include <GLFW/glfw3.h>
#include <glm/gtx/quaternion.hpp>


Player::Player(ClientNetwork* network, EasyModel* model, UserID_t uid, bool isMainPlayer, glm::vec3 position) 
: network(network), uid(uid), isMainPlayer(isMainPlayer), EasyEntity(model, position)
{
	
}

Player::~Player()
{

}

bool Player::Update(double _dt) 
{
	if (!animator && model->isRawDataLoadedToGPU && model->animations.size() > 0u)
		animator = new EasyAnimator(model->animations.at(0U));

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

bool useFakeRot{};
glm::quat fakeRot{ 1,0,0,0 };

glm::mat4x4 MainPlayer::TransformationMatrix() const
{
	return CreateTransformMatrix(transform.position, useFakeRot ? fakeRot : transform.rotationQuat, transform.scale);
}

void MainPlayer::UpdateVectors(TPCamera* camera)
{
	front = glm::normalize(transform.rotationQuat * glm::vec3(0, 0, 1));
	right = glm::normalize(glm::cross(front, glm::dvec3(0, 1, 0)));
	up = glm::normalize(glm::cross(right, front));
}

void MainPlayer::HandleAnimation(TPCamera* camera, double _dt)
{
	if (!animator)
		return;

	bool w = keyStates[GLFW_KEY_W];
	bool s = keyStates[GLFW_KEY_S];
	bool a = keyStates[GLFW_KEY_A];
	bool d = keyStates[GLFW_KEY_D];
	bool side = (a || d);
	bool move = (w || s);
	bool input = side || move;
	bool onlyMove = move && !side;
	bool onlySide = side && !move;
	bool movingSide = move && side;

	EasyAnimation* anim = model->animations[ANIM_IDLE];
	animator->SetMirror(false);

	if (w && !s)
	{
		anim = model->animations[ANIM_RUN_FORWARD];
		animator->SetMirror(false);
	}
	else if (s && !w)
	{
		anim = model->animations[ANIM_RUN_BACKWARD];
		animator->SetMirror(false);
	}
	else if (d && !a)
	{
		anim = model->animations[ANIM_STRAFE_RIGHT];
		animator->SetMirror(false);
	}
	else if (a && !d)
	{
		anim = model->animations[ANIM_STRAFE_RIGHT];
		animator->SetMirror(true);
	}

	animator->PlayAnimation(anim);
	animator->UpdateAnimation(_dt);
}

void MainPlayer::HandleMovement(TPCamera* camera, double _dt)
{
	if (camera->Rotating() && EasyMouse::IsButtonDown(GLFW_MOUSE_BUTTON_2))
	{
		if (glm::vec3 camForward = { camera->Front().x, 0.0f, camera->Front().z }; glm::length(camForward) >= 0.0001f)
		{
			targetTransform.rotationQuat = glm::rotation(glm::vec3(0, 0, 1), glm::normalize(camForward));
		}
	}

	UpdateVectors(camera);

	bool w = keyStates[GLFW_KEY_W];
	bool s = keyStates[GLFW_KEY_S];
	bool a = keyStates[GLFW_KEY_A];
	bool d = keyStates[GLFW_KEY_D];
	bool side = (a || d);
	bool move = (w || s);
	bool input = side || move;
	bool onlyMove = move && !side;
	bool onlySide = side && !move;
	bool movingSide = move && side;
	if (input)
	{
		useFakeRot = movingSide;
		
		glm::vec3 moveForward = glm::normalize(glm::vec3((float)front.x, 0.0f, (float)front.z));
		glm::vec3 moveRight = glm::normalize(glm::cross(moveForward, glm::vec3(0, 1, 0)));

		glm::vec3 dir(0.0f);
		if (w) dir += moveForward;
		if (s) dir -= moveForward;
		if (a) dir -= moveRight;
		if (d) dir += moveRight;

		if (glm::length(dir) > 0.0001f)
		{
			dir = glm::normalize(dir);
			if (movingSide)
				fakeRot = glm::angleAxis(std::atan2(dir.x, dir.z) + (s ? glm::pi<float>() : 0.f), glm::vec3(0, 1, 0));
		}

		double speed = s ? backwardsRunSpeed : runSpeed;
		targetTransform.position += dir * (float)speed * (float)_dt;
	}
	
	UpdateVectors(camera);

	transform.position += (targetTransform.position - transform.position) * 0.2f;
	transform.rotationQuat = glm::slerp(transform.rotationQuat, (glm::dot(transform.rotationQuat, targetTransform.rotationQuat) < 0.0f) ? -targetTransform.rotationQuat : targetTransform.rotationQuat,glm::clamp((float)_dt * 25.0f, 0.0f, 1.0f));
	
}

bool MainPlayer::Update(TPCamera* camera, double _dt)
{
	HandleMovement(camera, _dt);
	HandleAnimation(camera, _dt);
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