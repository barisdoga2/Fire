#include "Player.hpp"
#include <EasyIO.hpp>
#include <EasyModel.hpp>
#include <EasyAnimator.hpp>
#include <EasyAnimation.hpp>
#include <EasyCamera.hpp>

#include <GLFW/glfw3.h>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

#include "AnimationSM_unity.hpp"
#include "ClientNetwork.hpp"

class KeyDirection {
    static inline glm::ivec2 keyDir{};
public:
    static inline glm::ivec2 GetDir()
    {
        {
            static int ws = 0;
            const bool w = EasyKeyboard::IsKeyDown(GLFW_KEY_W);
            const bool s = EasyKeyboard::IsKeyDown(GLFW_KEY_S);
            int y = 0;
            if (w && s)
                y = -ws;
            else if (w || s)
                ws = y = w ? 1 : -1;
            else
                ws = 0;
            keyDir.y = y;
        }

        {
            static int ad = 0;
            const bool a = EasyKeyboard::IsKeyDown(GLFW_KEY_A);
            const bool d = EasyKeyboard::IsKeyDown(GLFW_KEY_D);
            int x = 0;
            if (a && d)
                x = -ad;
            else if (a || d)
                ad = x = a ? 1 : -1;
            else
                ad = 0;
            keyDir.x = x;
        }

        return keyDir;
    }
};

MainPlayer::MainPlayer(ClientNetwork* network, EasyModel* model, UserID_t uid, glm::vec3 position) : Player(network, model, uid, true, position)
{
    this->front = {0,1};
}

MainPlayer::~MainPlayer()
{
    
}

void MainPlayer::Move(TPCamera* camera, double _dt)
{
    //if(animator)
    //    animator->m_UpperAnimation = EasyKeyboard::IsKeyDown(GLFW_KEY_L) ? animator->animations[ANIM_AIM] : 0;
    float mouseRotate = 0.f;

    const bool mb1 = EasyMouse::IsButtonDown(GLFW_MOUSE_BUTTON_1);
    const bool mb2 = EasyMouse::IsButtonDown(GLFW_MOUSE_BUTTON_2);

    const glm::ivec2 rawKeyDir = KeyDirection::GetDir();

    const bool mouseMove = mb1 && mb2;
    const bool align = mouseMove || mb2;
    const glm::ivec2 moveDir = glm::ivec2(rawKeyDir.x, mouseMove ? 1 : rawKeyDir.y);

    const bool leftMove = moveDir.x < 0;
    const bool straigthMove = moveDir.y != 0;
    const bool sideMove = moveDir.x != 0;
    const bool diagonalMove = straigthMove && sideMove;
    const bool move = sideMove || straigthMove;
    const bool alignOnly = mb2 && !move;
    const bool onlySideMove = sideMove && !straigthMove;
    const bool onlyStraighthMove = !sideMove && straigthMove;
    const bool backwardsMove = moveDir.y == -1;
    const double speed = backwardsMove ? -backwardsRunSpeed : runSpeed;

    

    static bool wasAlignOnly = false;
    if (alignOnly || wasAlignOnly)
    {
        static float targetYaw = 0.f;
        
        wasAlignOnly = alignOnly;

        const glm::vec2 camFront = glm::normalize(glm::vec2(camera->Front().x, camera->Front().z));
        const float camYaw = glm::degrees(std::atan2(camFront.x, camFront.y));
        
        if (!alignOnly)
        {
            front = camFront;
            targetYaw = camYaw;
            mouseRotate = 0.f;
        }
        else
        {
            const float renderYaw = std::remainder(glm::degrees(std::atan2((transform.rotationQuat * glm::vec3(0, 0, 1)).x, (transform.rotationQuat * glm::vec3(0, 0, 1)).z)), 360.0f);
            if (abs(camYaw - targetYaw) > 30.f)
            {
                front = camFront;
                targetYaw = camYaw;
                mouseRotate = targetYaw > renderYaw ? 1.f : -1.f;
            }
            else if (abs(renderYaw - targetYaw) < 1.f)
            {
                mouseRotate = 0.f;
            }
            else
            {
                mouseRotate = targetYaw > renderYaw ? 1.f : -1.f;
            }
        }
        targetTransform.rotationQuat = glm::angleAxis(glm::radians(targetYaw), glm::vec3(0, 1, 0));

    }
    else if (move)
    {
        float targetYaw = glm::degrees(std::atan2(front.x, front.y));

        if (align)
            front = glm::normalize(glm::vec2(camera->Front().x, camera->Front().z));

        glm::vec2 combinedMoveDir{};
        if (onlyStraighthMove)
        {
            combinedMoveDir = front;
        }
        else if (onlySideMove)
        {
            combinedMoveDir = leftMove ? glm::vec2(-front.y, front.x) : glm::vec2(front.y, -front.x);
        }
        else if (diagonalMove)
        {
            float dgr = 45.f * (backwardsMove ? -1 : 1);

            targetYaw += moveDir.x * dgr;

            if(leftMove)
                combinedMoveDir = glm::vec2(front.x * glm::cos(dgr) - front.y * glm::sin(dgr), front.x * glm::sin(dgr) + front.y * glm::cos(dgr));
            else
                combinedMoveDir = glm::vec2(front.x * glm::cos(dgr) + front.y * glm::sin(dgr), -front.x * glm::sin(dgr) + front.y * glm::cos(dgr));
        }
        glm::vec2 moveDelta = (float)(speed * _dt) * combinedMoveDir;
        targetTransform.position += glm::vec3(moveDelta.x, 0.f, moveDelta.y);

        targetTransform.rotationQuat = glm::angleAxis(glm::radians(targetYaw), glm::vec3(0, 1, 0));
    }

    // After calculating targetTransform, send input to server
    glm::vec3 moveDirection = targetTransform.position - transform.position;
    if (glm::length(moveDirection) > 0.001f)
    {
        sPlayerInput* input = new sPlayerInput(
            network->session.uid,
            transform.position,
            glm::normalize(moveDirection)
        );
        network->GetSendCache().push_back(input);
    }

    const float pos_lerp_amt = glm::clamp((float)_dt * 12.0f, 0.0f, 1.0f);
    const float rot_lerp_amt = glm::clamp((float)_dt * 8.0f, 0.0f, 1.0f) * (mouseRotate ? 0.25f : 1.f);
    transform.position = lerp(transform.position, targetTransform.position, pos_lerp_amt);
    transform.rotationQuat = glm::slerp(transform.rotationQuat, targetTransform.rotationQuat, rot_lerp_amt);

    if (animator && stateManager)
    {
        //animator->LookAt("mixamorig:Head", camera->Front(), glm::normalize(glm::vec3(camera->Front().x, 0.f, camera->Front().z)), this->transform.rotationQuat, 0.6f, { -80.f, 50.f }, { -10.f, 10.f });
        //if(0 && EasyKeyboard::IsKeyDown(GLFW_KEY_L))
        //    animator->LookAt("mixamorig:Spine", camera->Front(), glm::normalize(glm::vec3(camera->Front().x, 0.f, camera->Front().z)), this->transform.rotationQuat, 0.6f, { -30.f, 30.f }, { 0.f, 0.f });

        stateManager->SetFloat("KeyDirX", (float)moveDir.x);
        stateManager->SetFloat("KeyDirY", (float)moveDir.y);
        static float lmouseRotate = mouseRotate;
        if (lmouseRotate != mouseRotate)
        {
            lmouseRotate = mouseRotate;
            std::cout << "mr: " << mouseRotate << "\n";
        }
        stateManager->SetFloat("IsStrafing", onlySideMove ? (float)moveDir.x : 0.f);
        stateManager->SetFloat("MouseRotate", (float)mouseRotate);
        stateManager->SetFloat("MoveAmount", mouseRotate != 0.f ? 1.f : std::abs((float)moveDir.x) + std::abs((float)moveDir.y));
        stateManager->SetBool("IsAiming", EasyKeyboard::IsKeyDown(GLFW_KEY_L));

        stateManager->Update((float)_dt);
        if (stateManager->GetBool("IsAiming"))
        {
            if (!animator->HasOverlay())
            {
                EasyAnimator::AnimationLayer layer;
                layer.clip = model->animations[ANIM_AIM];
                layer.mask = EasyAnimator::upperBodyMask(model);
                layer.weight = 0.01f;
                layer.targetWeight = 1.0f;
                layer.mirrorWeight = 0.0f;
                layer.mirrorTarget = onlySideMove && !leftMove ? -1.0f : onlySideMove && leftMove ? 1.f : 1.f;
                layer.blendSpeed = 10.0f;
                layer.mirrorBlendSpeed = 10.0f;
                animator->SetOverlayLayer(layer);
            }
            else
            {
                animator->SetOverlayTargetWeight(1.0f);
                animator->SetOverlayTargetMirror(onlySideMove && !leftMove ? -1.0f : onlySideMove && leftMove ? 1.f : 1.f);
            }
        }
        else
        {
            animator->SetOverlayTargetWeight(0.0f);
        }

        animator->Update((float)_dt);
    }
}

bool MainPlayer::Update(TPCamera* camera, double _dt)
{
    Move(camera, _dt);

    return EasyEntity::Update(_dt);
}

void MainPlayer::SetupAnimationSM()
{
    // --- PARAMETERS ---
    stateManager->AddFloat("MoveAmount", 0.f);
    stateManager->AddFloat("KeyDirX", 0.f);
    stateManager->AddFloat("KeyDirY", 0.f);
    stateManager->AddFloat("MouseRotate", 0.f);
    stateManager->AddBool("IsAiming", false);
    stateManager->AddFloat("IsStrafing", 0.f);

    // --- STATES ---
    stateManager->AddState("Idle", model->animations[ANIM_IDLE], true);
    stateManager->AddState("Back", model->animations[ANIM_RUN_BACKWARD], true);
    stateManager->AddState("Forward", model->animations[ANIM_RUN_FORWARD], true);
    stateManager->AddState("StrafeRight", model->animations[ANIM_STRAFE_RIGHT], true);
    stateManager->AddState("StrafeLeft", model->animations[ANIM_STRAFE_LEFT], true);
    stateManager->AddState("TurnRight", model->animations[ANIM_TURN_RIGHT], true);
    stateManager->AddState("TurnLeft", model->animations[ANIM_TURN_LEFT], true);

    stateManager->SetDefaultState("Idle");

    // --- TRANSITIONS ---
    float blends[] = { 0.8f, 0.8f, 0.8f, 0.8f, 0.8f, 0.8f, 0.8f, 0.8f, 0.8f, 0.8f, 0.8f, 0.8f, 0.8f, 0.8f, 0.8f, 0.8f, 0.8f, 0.8f, 0.8f };
    int slot = 0;

    // Any -> Idle  (KeyDirY == 0)
    auto aToIdle = stateManager->AddAnyTransition("Idle", blends[slot++]);
    stateManager->SetCondition(aToIdle, { "MoveAmount", AnimationSM::CompareOp::Equal , 0.f });

    // Any -> Aim
    //auto aToAim = stateManager->AddAnyTransition("Aim", blends[slot++]);
    //stateManager->AddCondition_Bool(aToAim, "IsAiming");

    // Any -> TurnLeft|TurnRight
    auto aToTurnR = stateManager->AddAnyTransition("TurnRight", blends[slot++]);
    stateManager->SetCondition(aToTurnR, { "MouseRotate", AnimationSM::CompareOp::Less, 0.f });
    auto aToTurnL = stateManager->AddAnyTransition("TurnLeft", blends[slot++]);
    stateManager->SetCondition(aToTurnL, { "MouseRotate", AnimationSM::CompareOp::Greater, 0.f });

    // Any -> StrafeRight|StrafeLeft
    auto aToStrafeR = stateManager->AddAnyTransition("StrafeRight", blends[slot++]);
    stateManager->SetCondition(aToStrafeR, { "IsStrafing", AnimationSM::CompareOp::Less, 0.f });
    auto aToStrafeL = stateManager->AddAnyTransition("StrafeLeft", blends[slot++]);
    stateManager->SetCondition(aToStrafeL, { "IsStrafing", AnimationSM::CompareOp::Greater, 0.f });

    // Any -> Forward
    auto aToForward = stateManager->AddAnyTransition("Forward", blends[slot++]);
    stateManager->SetCondition(aToForward, { "KeyDirY", AnimationSM::CompareOp::Greater, 0.f });

    // Any -> Back
    auto aToBack = stateManager->AddAnyTransition("Back", blends[slot++]);
    stateManager->SetCondition(aToBack, { "KeyDirY", AnimationSM::CompareOp::Less, 0.f });

    stateManager->Start();
}

glm::mat4x4 MainPlayer::TransformationMatrix() const
{
    return CreateTransformMatrix(transform.position, transform.rotationQuat, transform.scale);
}

void MainPlayer::AssetReadyCallback()
{
    Player::AssetReadyCallback();
    if (stateManager)
        SetupAnimationSM();
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
