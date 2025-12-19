#pragma once

#include <deque>
#include <EasyIO.hpp>
#include <EasyEntity.hpp>
#include <../GameServer/GameServer.hpp>
#include <../GameServer/ServerNet.hpp>

enum AnimID : uint32_t {
	ANIM_IDLE = 0,
	ANIM_RUN_BACKWARD,
	ANIM_RUN_FORWARD,
	ANIM_STRAFE_RIGHT,
	ANIM_TURN_RIGHT,
};

class TPCamera;
class EasyModel;
class ClientNetwork;
class Player : public EasyEntity, MouseListener, KeyboardListener {
public:
	const bool isMainPlayer;
	const UserID_t uid;
	ClientNetwork* network;

	EasyTransform targetTransform{};

	Player(ClientNetwork* network, EasyModel* model, UserID_t uid, bool isMainPlayer, glm::vec3 position);
	~Player();

	bool Update(double _dt) override;

	bool button_callback(const MouseData& data) override;
	bool scroll_callback(const MouseData& data) override;
	bool move_callback(const MouseData& data) override;
	bool key_callback(const KeyboardData& data) override;
	bool char_callback(const KeyboardData& data) override;
};


class MainPlayer : public Player {
public:
	bool keyStates[MAX_KEYS] = {false};

	glm::dvec3 front{};
	glm::dvec3 up{};
	glm::dvec3 right{};

	double runSpeed = 5.f;
	double backwardsRunSpeed = 2.5f;

	MainPlayer(ClientNetwork* network, EasyModel* model, UserID_t uid, glm::vec3 position);
	~MainPlayer();

	glm::mat4x4 TransformationMatrix() const override;
	void UpdateVectors(TPCamera* camera);
	void HandleMovement(TPCamera* camera, double _dt);
	void HandleAnimation(TPCamera* camera, double _dt);
	bool Update(TPCamera* camera, double _dt);

	//bool button_callback(const MouseData& data) override;
	//bool scroll_callback(const MouseData& data) override;
	//bool move_callback(const MouseData& data) override;
	bool key_callback(const KeyboardData& data) override;
	//bool char_callback(const KeyboardData& data) override;
};
