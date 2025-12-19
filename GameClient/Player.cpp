#include "Player.hpp"
#include <EasyIO.hpp>
#include <EasyModel.hpp>
#include <EasyAnimator.hpp>
#include <EasyAnimation.hpp>
#include <EasyCamera.hpp>

#include <GLFW/glfw3.h>
#include <glm/gtx/quaternion.hpp>

#include "ClientNetwork.hpp"
#include "AnimationSM.hpp"

Player::Player(ClientNetwork* network, EasyModel* model, UserID_t uid, bool isMainPlayer, glm::vec3 position) 
: network(network), uid(uid), isMainPlayer(isMainPlayer), sm(), EasyEntity(model, position)
{
	
}

Player::~Player()
{

}

void Player::AssetReadyCallback()
{
	EasyEntity::AssetReadyCallback();
	if (animator)
	{
		animator->PlayAnimation(model->animations[ANIM_IDLE]);
		sm = new AnimationSM(animator);
	}
}

bool Player::Update(double _dt) 
{
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



static inline bool StepYawToward(glm::quat& fixedQt, const glm::quat& dynamicQt, float stepDeg = 30.0f)
{
	// "Subtract" quats: rotation that takes fixed -> dynamic
	glm::quat delta = glm::normalize(dynamicQt * glm::inverse(fixedQt));

	// Axis-angle from delta (pure quat math)
	float w = glm::clamp(delta.w, -1.0f, 1.0f);
	float angle = 2.0f * std::acos(w);                 // [0..pi*2]
	float s = std::sqrt(glm::max(0.0f, 1.0f - w * w));   // sin(angle/2)

	if (s < 1e-6f) return false; // nearly identical

	glm::vec3 axis(delta.x / s, delta.y / s, delta.z / s);

	// Signed yaw contribution in radians (project axis onto Y)
	float yawRad = angle * axis.y;

	// Wrap to [-pi, +pi]
	if (yawRad > glm::pi<float>()) yawRad -= glm::two_pi<float>();
	if (yawRad < -glm::pi<float>()) yawRad += glm::two_pi<float>();

	float yawDeg = glm::degrees(yawRad);

	if (std::abs(yawDeg) < stepDeg) return false;

	float signedStep = (yawDeg > 0.0f) ? stepDeg : -stepDeg;
	glm::quat stepQt = glm::angleAxis(glm::radians(signedStep), glm::vec3(0, 1, 0));

	fixedQt = glm::normalize(stepQt * fixedQt); // commit step
	return true;
}

bool diagonalMoving{};
glm::quat diagonalMoveStartRot{};

bool stepRotating{};
glm::quat stepRotateStartRot{};

MainPlayer::MainPlayer(ClientNetwork* network, EasyModel* model, UserID_t uid, glm::vec3 position) : Player(network, model, uid, true, position)
{}

MainPlayer::~MainPlayer()
{}

void MainPlayer::AssetReadyCallback()
{
	Player::AssetReadyCallback();
}

bool MainPlayer::Update(TPCamera* camera, double _dt)
{
	// Input Flags
	bool b1 = buttonStates[GLFW_MOUSE_BUTTON_1];
	bool b2 = buttonStates[GLFW_MOUSE_BUTTON_2];
	bool mouseButtonRotate = camera->Rotating() && b2;
	bool mouseButtonMove = b1 && b2;

	bool w = keyStates[GLFW_KEY_W] || mouseButtonMove;
	bool s = keyStates[GLFW_KEY_S];
	bool a = keyStates[GLFW_KEY_A];
	bool d = keyStates[GLFW_KEY_D];
	bool side = (a || d);
	bool move = (w || s);
	bool input = side || move;
	bool onlyMove = move && !side;
	bool onlySide = side && !move;
	bool movingSide = move && side;

	// Rotation(by camera)
	bool playStepAnimation{};
	{
		if (!input && mouseButtonRotate) // Only MOUSE_BUTTON_2 pressed.
		{
			if (!stepRotating)
			{
				stepRotateStartRot = targetTransform.rotationQuat;
				stepRotating = true;
			}
			else if(StepYawToward(stepRotateStartRot, targetTransform.rotationQuat))
			{
				playStepAnimation = true;
			}
		}
		else if (stepRotating)
		{
			stepRotating = false;
			playStepAnimation = true;
		}
		
		if (glm::vec3 camForward = { camera->Front().x, 0.0f, camera->Front().z }; (mouseButtonRotate && glm::length(camForward) >= 0.0001f))
		{
			targetTransform.rotationQuat = glm::rotation(glm::vec3(0, 0, 1), glm::normalize(camForward));
		}
		UpdateVectors(camera);
	}

	// Movement
	{
		if (input)
		{
			diagonalMoving = movingSide;

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
					diagonalMoveStartRot = glm::angleAxis(std::atan2(dir.x, dir.z) + (s ? glm::pi<float>() : 0.f), glm::vec3(0, 1, 0));
			}

			double speed = s ? backwardsRunSpeed : runSpeed;
			targetTransform.position += dir * (float)speed * (float)_dt;
		}
	}

	// Interpolation
	{
		transform.position += (targetTransform.position - transform.position) * 0.2f;
		transform.rotationQuat = glm::slerp(transform.rotationQuat, (glm::dot(transform.rotationQuat, targetTransform.rotationQuat) < 0.0f) ? -targetTransform.rotationQuat : targetTransform.rotationQuat, glm::clamp((float)_dt * 25.0f, 0.0f, 1.0f));
	}

	// Animating
	{
		if (animator)
		{
			if (w && !s)
			{
				animator->PlayAnimation(model->animations[ANIM_RUN_FORWARD]);
			}
			else if (s && !w)
			{
				animator->PlayAnimation(model->animations[ANIM_RUN_BACKWARD]);
			}
			else if (d && !a)
			{
				animator->PlayAnimation(model->animations[ANIM_STRAFE_RIGHT]);
			}
			else if (a && !d)
			{
				animator->PlayAnimation(model->animations[ANIM_STRAFE_RIGHT]);
			}
			else if (playStepAnimation)
			{
				animator->PlayAnimation(model->animations[ANIM_TURN_RIGHT]);
			}
			else
			{
				animator->PlayAnimation(model->animations[ANIM_IDLE]);
			}

			animator->SetMirror(a && !d ? true : false);

			animator->UpdateAnimation(_dt);
		}
	}

	return Player::Update(_dt);
}

void MainPlayer::UpdateVectors(TPCamera* camera)
{
	front = glm::normalize(transform.rotationQuat * glm::vec3(0, 0, 1));
	right = glm::normalize(glm::cross(front, glm::dvec3(0, 1, 0)));
	up = glm::normalize(glm::cross(right, front));
}

glm::mat4x4 MainPlayer::TransformationMatrix() const
{
	glm::quat rot{};

	if (diagonalMoving)
		rot = diagonalMoveStartRot;
	else if (stepRotating)
		rot = stepRotateStartRot;
	else
		rot = transform.rotationQuat;

	return CreateTransformMatrix(transform.position, rot, transform.scale);
}

bool MainPlayer::key_callback(const KeyboardData& data)
{
	bool captured = false;

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
		captured = true;
	}

	return captured;
}

bool MainPlayer::button_callback(const MouseData& data)
{
	// TPCamera also needs to capture, so do not capture here, it will for sure capture both
	bool captured = false;

	if (data.button.button == GLFW_MOUSE_BUTTON_1 || data.button.button == GLFW_MOUSE_BUTTON_2)
	{
		if (data.button.action == GLFW_PRESS || data.button.action == GLFW_REPEAT)
		{
			buttonStates[data.button.button] = true;
		}
		else if (data.button.action == GLFW_RELEASE)
		{
			buttonStates[data.button.button] = false;
		}
	}

	return captured;
}
