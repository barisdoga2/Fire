#pragma once

#include <deque>
#include <EasyIO.hpp>
#include <EasyEntity.hpp>
#include <../GameServer/GameServer.hpp>
#include <../GameServer/ServerNet.hpp>

enum AnimID {
	ANIM_IDLE = 0,
	ANIM_RUN_BACKWARD,
	ANIM_RUN_FORWARD,
	ANIM_STRAFE_RIGHT,
	ANIM_STRAFE_LEFT,
	ANIM_TURN_RIGHT,
	ANIM_TURN_LEFT,
	ANIM_AIM,
	ANIM_MAX
};

class TPCamera;
class EasyModel;
class ClientNetwork;
class AnimationSM;
class Player : public EasyEntity, MouseListener, KeyboardListener {
private:
	const bool isMainPlayer;
	const UserID_t uid;
	ClientNetwork* network;

protected:
	EasyTransform targetTransform{};
	AnimationSM* stateManager{};

	glm::vec2 front{};

	double runSpeed = 5.f;
	double backwardsRunSpeed = 2.5f;

public:
	Player(ClientNetwork* network, EasyModel* model, UserID_t uid, bool isMainPlayer, glm::vec3 position);
	~Player();

	bool Update(double _dt) override;
	void AssetReadyCallback() override;

	UserID_t UserID() const { return uid; };

	bool button_callback(const MouseData& data) override;
	bool scroll_callback(const MouseData& data) override;
	bool move_callback(const MouseData& data) override;
	bool key_callback(const KeyboardData& data) override;
	bool char_callback(const KeyboardData& data) override;
};


class MainPlayer : public Player {
private:
	bool keyStates[MAX_KEYS] = { false };
	bool buttonStates[MAX_BUTTONS] = { false };

public:
	MainPlayer(ClientNetwork* network, EasyModel* model, UserID_t uid, glm::vec3 position);
	~MainPlayer();
	
	void Move(TPCamera* camera, double _dt);
	bool Update(TPCamera* camera, double _dt);

	void AssetReadyCallback() override;
	glm::mat4x4 TransformationMatrix() const override;
	void SetupAnimationSM();

	bool button_callback(const MouseData& data) override;
	bool key_callback(const KeyboardData& data) override;
};
