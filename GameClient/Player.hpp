#pragma once

#include <EasyModel.hpp>
#include <EasyIO.hpp>
#include <../GameServer/GameServer.hpp>

class Player : public EasyEntity, MouseListener, KeyboardListener {
public:
	const bool isMainPlayer;
	const UserID_t uid;

	Player(EasyModel* model, UserID_t uid, bool isMainPlayer, glm::vec3 position);

	~Player();

	bool Update(double _dt) override;

	bool mouse_callback(const MouseData& md) override;
	bool scroll_callback(const MouseData& md) override;
	bool cursorMove_callback(const MouseData& md) override;
	bool key_callback(const KeyboardData& data) override;
	bool character_callback(const KeyboardData& data) override;

};
