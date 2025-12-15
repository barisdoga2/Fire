#include "Player.hpp"
#include <EasyModel.hpp>
#include <EasyAnimator.hpp>
#include <EasyAnimation.hpp>
#include "ClientNetwork.hpp"



static sPlayerInput MakeInput(Player& p, double dt)
{
    glm::vec3 dir(0.f);

    if (EasyKeyboard::IsKeyDown('W')) dir.z += 1.f;
    if (EasyKeyboard::IsKeyDown('S')) dir.z -= 1.f;
    if (EasyKeyboard::IsKeyDown('D')) dir.x += 1.f;
    if (EasyKeyboard::IsKeyDown('A')) dir.x -= 1.f;

    if (glm::length(dir) > 0.f)
        dir = glm::normalize(dir);

    const uint32_t dtMs = (uint32_t)glm::clamp(dt * 1000.0, 0.0, 255.0); // şimdilik clamp
    return sPlayerInput(p.uid, p.nextInputSeq++, dtMs, dir, /*clientTimeMs*/0ULL);
}

static bool InputChanged(const sPlayerInput& a, const sPlayerInput& b)
{
    return glm::distance(a.moveDir, b.moveDir) > 0.001f;
}

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

    sPlayerInput in = MakeInput(*this, _dt);

    // 1) prediction her frame
    ApplyInput(in);
    pendingInputs.push_back(in);

    // 2) input send (SABİT RATE)
    constexpr double SEND_INTERVAL = 0.05; // 50ms = 20Hz
    sendAccum += _dt;

    if (network && sendAccum >= SEND_INTERVAL)
    {
        network->GetSendCache().push_back(
            new sPlayerInput(in.uid, in.inputSeq, in.dtMs, in.moveDir, in.clientTimeMs)
        );
        sendAccum = 0.0;
    }

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

void Player::ApplyInput(const sPlayerInput& in)
{
    const float speed = 5.0f;
    const float dts = (float)in.dtMs / 1000.f;

    transform.position += in.moveDir * (speed * (float)dts); // veya: position += ...
}

void Player::Reconcile()
{
    transform.position = serverState.position;

    auto it = pendingInputs.begin();
    while (it != pendingInputs.end())
    {
        if (it->inputSeq <= serverState.lastInputSequence)
            it = pendingInputs.erase(it);
        else
            ++it;
    }

    for (const auto& in : pendingInputs)
        ApplyInput(in);
}

void Player::PushSnapshot(const sPlayerState& s)
{
    snapBuf.push_back(NetSnapshot{ (uint64_t)s.serverTimestamp, s.position, s.velocity });

    // buffer şişmesin
    while (snapBuf.size() > 32)
        snapBuf.pop_front();
}

static glm::vec3 Lerp(const glm::vec3& a, const glm::vec3& b, float t)
{
    return a + (b - a) * t;
}

void Player::UpdateRemoteInterpolation(uint64_t nowClientMs)
{
    if (isMainPlayer) return;

    constexpr uint64_t interpDelayMs = 200; // 2 snapshot gerisi
    const uint64_t renderTime = (nowClientMs > interpDelayMs) ? (nowClientMs - interpDelayMs) : 0;

    // renderTime'dan eski olanları at (en az 2 kalsın)
    while (snapBuf.size() >= 2 && snapBuf[1].serverTimeMs <= renderTime)
        snapBuf.pop_front();

    if (snapBuf.size() == 0) return;

    if (snapBuf.size() == 1)
    {
        // tek snapshot varsa snap (ya da extrapolate)
        transform.position = snapBuf[0].pos;
        return;
    }

    const auto& a = snapBuf[0];
    const auto& b = snapBuf[1];

    const uint64_t t0 = a.serverTimeMs;
    const uint64_t t1 = b.serverTimeMs;

    float t = 0.f;
    if (t1 > t0)
        t = float(double(renderTime - t0) / double(t1 - t0));

    t = glm::clamp(t, 0.f, 1.f);
    transform.position = Lerp(a.pos, b.pos, t);
}
