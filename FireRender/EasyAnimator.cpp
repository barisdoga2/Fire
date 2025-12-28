#include "pch.h"
#include "EasyAnimator.hpp"
#include "EasyAnimation.hpp"
#include "EasyUtils.hpp"
#include "EasyRender.hpp"

#include <iostream>
#include <algorithm>
#include <cmath>

#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/compatibility.hpp>
#include <glm/gtx/quaternion.hpp>
#include <assimp/scene.h>


struct LookAtCmd
{
    std::string boneName;
    glm::vec3 cameraFrontBase;
    glm::vec3 cameraFront;
    glm::quat characterRot;
    float weight;
    std::pair<float, float> yawLimits;
    std::pair<float, float> pitchLimits;


    float yaw{}, pitch{};
    float targetYaw{}, targetPitch{};
};
LookAtCmd lookAt;

static inline float ClampDeg(float v, float minD, float maxD)
{
    return std::max(minD, std::min(maxD, v));
}

static const std::unordered_map<std::string, float> upperBodyMask = {
{"mixamorig:Hips", 0.0f},
{"mixamorig:Spine", 0.3f},
{"mixamorig:Spine1", 0.6f},
{"mixamorig:Spine2", 1.0f},

{"mixamorig:Bowl5", 1.0f},
{"mixamorig:Bowl1", 1.0f},
{"mixamorig:Bowl6", 1.0f},
{"mixamorig:Bowl2", 1.0f},
{"mixamorig:Bowl7", 1.0f},
{"mixamorig:Bowl3", 1.0f},
{"mixamorig:Bowl8", 1.0f},
{"mixamorig:Bowl4", 1.0f},

{"mixamorig:Neck", 1.0f},
{"mixamorig:Head", 1.0f},
{"mixamorig:Leye", 1.0f},
{"mixamorig:Reye", 1.0f},
{"mixamorig:Head", 1.0f},
{"mixamorig:HeadTop_End", 1.0f},
{"mixamorig:LeftShoulder", 1.0f},
{"mixamorig:LeftArm", 1.0f},
{"mixamorig:LeftForeArm", 1.0f},
{"mixamorig:LeftHand", 1.0f},
{"mixamorig:RightShoulder", 1.0f},
{"mixamorig:RightArm", 1.0f},
{"mixamorig:RightForeArm", 1.0f},
{"mixamorig:RightHand", 1.0f},

{"mixamorig:RightHandPinky1", 1.0f},
{"mixamorig:RightHandPinky2", 1.0f},
{"mixamorig:RightHandPinky3", 1.0f},
{"mixamorig:RightHandPinky4", 1.0f},
{"mixamorig:RightHandRing1", 1.0f},
{"mixamorig:RightHandRing2", 1.0f},
{"mixamorig:RightHandRing3", 1.0f},
{"mixamorig:RightHandRing4", 1.0f},
{"mixamorig:RightHandMiddle1", 1.0f},
{"mixamorig:RightHandMiddle2", 1.0f},
{"mixamorig:RightHandMiddle3", 1.0f},
{"mixamorig:RightHandMiddle4", 1.0f},
{"mixamorig:RightHandIndex1", 1.0f},
{"mixamorig:RightHandIndex2", 1.0f},
{"mixamorig:RightHandIndex3", 1.0f},
{"mixamorig:RightHandIndex4", 1.0f},
{"mixamorig:RightHandThumb1", 1.0f},
{"mixamorig:RightHandThumb2", 1.0f},
{"mixamorig:RightHandThumb3", 1.0f},
{"mixamorig:RightHandThumb4", 1.0f},
{"mixamorig:LeftHandPinky1", 1.0f},
{"mixamorig:LeftHandPinky2", 1.0f},
{"mixamorig:LeftHandPinky3", 1.0f},
{"mixamorig:LeftHandPinky4", 1.0f},
{"mixamorig:LeftHandRing1", 1.0f},
{"mixamorig:LeftHandRing2", 1.0f},
{"mixamorig:LeftHandRing3", 1.0f},
{"mixamorig:LeftHandRing4", 1.0f},
{"mixamorig:LeftHandMiddle1", 1.0f},
{"mixamorig:LeftHandMiddle2", 1.0f},
{"mixamorig:LeftHandMiddle3", 1.0f},
{"mixamorig:LeftHandMiddle4", 1.0f},
{"mixamorig:LeftHandIndex1", 1.0f},
{"mixamorig:LeftHandIndex2", 1.0f},
{"mixamorig:LeftHandIndex3", 1.0f},
{"mixamorig:LeftHandIndex4", 1.0f},
{"mixamorig:LeftHandThumb1", 1.0f},
{"mixamorig:LeftHandThumb2", 1.0f},
{"mixamorig:LeftHandThumb3", 1.0f},
{"mixamorig:LeftHandThumb4", 1.0f}
};

static const std::unordered_map<std::string, float> lowerBodyMask = {
    {"mixamorig:Hips", 1.0f},
    {"mixamorig:LeftUpLeg", 1.0f},
    {"mixamorig:LeftLeg", 1.0f},
    {"mixamorig:LeftFoot", 1.0f},
    {"mixamorig:RightUpLeg", 1.0f},
    {"mixamorig:RightLeg", 1.0f},
    {"mixamorig:RightFoot", 1.0f},
    {"mixamorig:RightToeBase", 1.0f},
    {"mixamorig:RightToe_End", 1.0f},
    {"mixamorig:LeftToeBase", 1.0f},
    {"mixamorig:LeftToe_End", 1.0f},
};

EasyAnimator::EasyAnimator(const std::vector<EasyAnimation*>& animations) : animations(animations)
{
    m_CurrentTime = 0.0;
    m_CurrentAnimation = nullptr;

    m_FinalBoneMatrices.reserve(200);

    for (int i = 0; i < 200; i++)
        m_FinalBoneMatrices.push_back(glm::mat4x4(1.0f));
}

void EasyAnimator::PlayAnimation(EasyAnimation* pAnimation)
{
    std::cout << "PlayAnimation " << pAnimation->m_Name << "\n";

    m_CurrentAnimation = pAnimation;
    m_CurrentTime = 0.0;
}

void EasyAnimator::PlayAnimation(uint8_t aID)
{
    if (aID < animations.size())
        PlayAnimation(animations.at(aID));
}

void EasyAnimator::BlendTo(uint8_t aID, double duration)
{
    EasyAnimation* pAnimation{};
    if (aID < animations.size())
    {
        pAnimation = animations.at(aID);
        BlendTo(pAnimation, duration);
    }
}

void EasyAnimator::BlendTo(EasyAnimation* next, double duration)
{
    std::cout << "BlendTo " << next->m_Name << "\n";

    m_NextAnimation = next;
    m_BlendTime = 0.0;
    m_BlendDuration = duration;
    m_IsBlending = true;
}

void EasyAnimator::LookAt(const std::string& boneName, const glm::vec3& cameraFrontBase, const glm::vec3& cameraFront, glm::quat characterRot, float weight, std::pair<float, float> yawLimits, std::pair<float, float> pitchLimits)
{
    if (!m_CurrentAnimation || weight <= 0.0001f)
        return;

    lookAt.boneName = boneName;
    lookAt.cameraFrontBase = cameraFrontBase;
    lookAt.characterRot = characterRot;
    lookAt.cameraFront = cameraFront;
    lookAt.weight = weight;
    lookAt.yawLimits = yawLimits;
    lookAt.pitchLimits = pitchLimits;
}

void EasyAnimator::UpdateLayered(double dt)
{

}

void EasyAnimator::UpdateAnimation(double dt)
{
    if (!m_CurrentAnimation)
        return;

    m_CurrentTime += m_CurrentAnimation->m_TicksPerSecond * dt * m_PlaybackSpeed;
    m_CurrentTime = fmod(m_CurrentTime, m_CurrentAnimation->m_Duration);

    std::vector<glm::mat4> baseBones(200, glm::mat4(1.0f));
    CalculateBoneTransform(m_CurrentAnimation, &m_CurrentAnimation->m_RootNode, glm::mat4(1.0f), baseBones, m_CurrentTime, mirror);

    static double m_UpperCurrentTime = 0.0;
    if (m_UpperAnimation)
    {
        m_UpperCurrentTime += m_UpperAnimation->m_TicksPerSecond * dt * m_PlaybackSpeed;
        m_UpperCurrentTime = fmod(m_UpperCurrentTime, m_UpperAnimation->m_Duration);

        std::vector<glm::mat4> aimBones(200, glm::mat4(1.0f));
        CalculateBoneTransform(m_UpperAnimation, &m_UpperAnimation->m_RootNode, glm::mat4(1.0f), aimBones, m_UpperCurrentTime, mirror);

        std::vector<glm::mat4> blendBones(200, glm::mat4(1.0f));
        for (auto& [boneName, info] : m_CurrentAnimation->m_BoneInfoMap)
        {
            int id = info.id;
            float mask = 0.0f;
            if (auto it = upperBodyMask.find(boneName); it != upperBodyMask.end())
                mask = it->second;

            float lowerW = 1.0f - mask;
            float upperW = mask;

            blendBones[id] = baseBones[id] * lowerW + aimBones[id] * upperW;
        }
        baseBones = blendBones;
    }
    else
    {
        m_UpperCurrentTime = 0.0;
    }

    
    if (m_IsBlending && m_NextAnimation)
    {
        m_BlendTime += dt;

        double alpha = std::min(m_BlendTime / m_BlendDuration, 1.0);
        double nextTime = fmod(m_CurrentTime, m_NextAnimation->m_Duration);

        std::vector<glm::mat4> nextAnimationBones(200, glm::mat4(1.0f));
        CalculateBoneTransform(m_NextAnimation, &m_NextAnimation->m_RootNode, glm::mat4(1.0f), nextAnimationBones, nextTime, mirror);

        for (size_t i = 0; i < m_FinalBoneMatrices.size(); ++i)
            baseBones[i] = baseBones[i] * (1.0f - (float)alpha) + nextAnimationBones[i] * (float)alpha;

        if (alpha >= 1.0)
        {
            m_CurrentAnimation = m_NextAnimation;
            m_NextAnimation = nullptr;
            m_IsBlending = false;
        }
    }

    m_FinalBoneMatrices = baseBones;
}

void EasyAnimator::CalculateBoneTransform(EasyAnimation* animation, const AssimpNodeData* node,const glm::mat4& parentTransform, std::vector<glm::mat4>& outMatrices, double dt, bool mirror)
{
    EasyBone* Bone = nullptr;
    if (auto it = animation->boneLookup.find(node->name); it != animation->boneLookup.end())
        Bone = it->second;

    glm::mat4 nodeTransform = node->transformation;
    if (Bone)
    {
        Bone->Update(dt);
        EasyTransformExt transform(Bone->GetLocalTransform());

        if (lookAt.boneName == node->name)
        {
            const glm::quat characterWorldRot = lookAt.characterRot;
            const glm::vec3 charSpaceDir = glm::inverse(characterWorldRot) * glm::normalize(lookAt.cameraFront);
            const glm::quat parentWorldRot = glm::quat_cast(parentTransform);
            const glm::vec3 localDir = glm::inverse(parentWorldRot) * charSpaceDir;

            float yAmt = glm::degrees(std::atan2(localDir.x, localDir.z));
            float xAmt = glm::degrees(std::atan2(localDir.y, glm::length(glm::vec2(localDir.x, localDir.z))));
            xAmt = glm::clamp(xAmt, lookAt.pitchLimits.first, lookAt.pitchLimits.second);
            yAmt = glm::clamp(yAmt, lookAt.yawLimits.first, lookAt.yawLimits.second);

            lookAt.targetPitch = xAmt;
            lookAt.targetYaw = yAmt;

            lookAt.yaw = glm::lerp(lookAt.yaw, lookAt.targetYaw, 0.025f);
            lookAt.pitch = glm::lerp(lookAt.pitch, lookAt.targetPitch, 0.025f);

            const glm::quat rot = glm::angleAxis(glm::radians(lookAt.pitch), glm::vec3(1, 0, 0)) * glm::angleAxis(glm::radians(lookAt.yaw), glm::vec3(0, 1, 0));
            transform.rotationQuat = glm::normalize(transform.rotationQuat * rot);
        }


        if (mirror)
        {
            transform.position.x = -transform.position.x;
            transform.rotationQuat.y = -transform.rotationQuat.y;
            transform.rotationQuat.z = -transform.rotationQuat.z;
        }

        nodeTransform = transform.TransformationMatrix();
    }

    glm::mat4 globalTransform = parentTransform * nodeTransform;
    auto& boneInfoMap = animation->m_BoneInfoMap;
    if (auto it = boneInfoMap.find(node->name); it != boneInfoMap.end())
        outMatrices[it->second.id] = globalTransform * it->second.offset;

    for (int i = 0; i < node->childrenCount; ++i)
        CalculateBoneTransform(animation, &node->children[i], globalTransform, outMatrices, dt, mirror);
}

float EasyAnimator::GetNormalizedTime() const
{
    if (!m_CurrentAnimation || m_CurrentAnimation->m_Duration <= 0.0f)
        return 0.0f;

    double nt = m_CurrentTime / m_CurrentAnimation->m_Duration;
    if (nt < 0.0)
        nt = 0.0;
    else if (nt > 1.0)
        nt = 1.0;

    return (float)nt;
}
