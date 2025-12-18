#pragma once

#include <deque>
#include <EasyIO.hpp>
#include <EasyEntity.hpp>
#include <../GameServer/GameServer.hpp>
#include <../GameServer/ServerNet.hpp>


class EasyModel;
class ClientNetwork;
class Player : public EasyEntity, MouseListener, KeyboardListener {
public:
	const bool isMainPlayer;
	const UserID_t uid;
	ClientNetwork* network;

	glm::vec3 targetPosition;
	glm::vec3 direction{};

	float speed = 5.0f;

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

	MainPlayer(ClientNetwork* network, EasyModel* model, UserID_t uid, glm::vec3 position);
	~MainPlayer();

	bool Update(double _dt) override;

	//bool button_callback(const MouseData& data) override;
	//bool scroll_callback(const MouseData& data) override;
	//bool move_callback(const MouseData& data) override;
	bool key_callback(const KeyboardData& data) override;
	//bool char_callback(const KeyboardData& data) override;
};
