#pragma once

#include <deque>
#include <EasyIO.hpp>
#include <EasyEntity.hpp>
#include <../GameServer/GameServer.hpp>
#include <../GameServer/ServerNet.hpp>

struct NetSnapshot
{
	uint64_t serverTimeMs{};
	glm::vec3 pos{};
	glm::vec3 vel{};
};


class EasyModel;
class ClientNetwork;
class Player : public EasyEntity, MouseListener, KeyboardListener {
public:
	const bool isMainPlayer;
	const UserID_t uid;
	ClientNetwork* network;

	Player(ClientNetwork* network, EasyModel* model, UserID_t uid, bool isMainPlayer, glm::vec3 position);
	~Player();

	bool Update(double _dt) override;

	bool mouse_callback(const MouseData& md) override;
	bool scroll_callback(const MouseData& md) override;
	bool cursorMove_callback(const MouseData& md) override;
	bool key_callback(const KeyboardData& data) override;
	bool character_callback(const KeyboardData& data) override;



	SequenceID_t nextInputSeq = 0U;
	std::vector<sPlayerInput> pendingInputs;
	sPlayerState serverState{};
	sPlayerInput lastSentInput{};
	bool hasLastSentInput{ false };
	double sendAccum{ 0.0 };

	void ApplyInput(const sPlayerInput& in);
	void Reconcile();

	std::deque<NetSnapshot> snapBuf;
	void PushSnapshot(const sPlayerState& s);
	void UpdateRemoteInterpolation(uint64_t nowClientMs);
};
