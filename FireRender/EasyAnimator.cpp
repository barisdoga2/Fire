#include "pch.h"
#include "EasyAnimator.hpp"
#include "EasyAnimation.hpp"
#include "EasyUtils.hpp"

#include <algorithm>
#include <cmath>

#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/compatibility.hpp>
#include <glm/gtx/quaternion.hpp>
#include <assimp/scene.h>



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

EasyAnimator::EasyAnimator(const std::vector<EasyAnimation*>& animations)
{
    m_CurrentTime = 0.0;
    m_CurrentAnimation = nullptr;

    m_FinalBoneMatrices.reserve(200);

    for (int i = 0; i < 200; i++)
        m_FinalBoneMatrices.push_back(glm::mat4x4(1.0f));
}

void EasyAnimator::PlayAnimation(EasyAnimation* pAnimation)
{
    if (m_CurrentAnimation != pAnimation)
    {
        m_CurrentAnimation = pAnimation;
        m_CurrentTime = 0.0;
    }
}

void EasyAnimator::PlayAnimation(uint8_t aID)
{
    EasyAnimation* pAnimation{};
    if (aID < animations.size())
        pAnimation = animations.at(aID);
    if (m_CurrentAnimation != pAnimation)
    {
        m_CurrentAnimation = pAnimation;
        m_CurrentTime = 0.0;
    }
}

void EasyAnimator::BlendTo(EasyAnimation* next, double duration)
{
    if (!next || next == m_CurrentAnimation || m_IsBlending)
        return;
    m_NextAnimation = next;
    m_BlendTime = 0.0;
    m_BlendDuration = duration;
    m_IsBlending = true;
}

void EasyAnimator::UpdateLayered(EasyAnimation* lowerAnim, EasyAnimation* upperAnim, bool aiming, double dt)
{
    if (!lowerAnim || !upperAnim) return;

	// Smoothly change blend factor
	float target = aiming ? 1.0f : 0.0f;
	upperBodyBlend = glm::mix(upperBodyBlend, target, (float)(dt * 8.0)); // smooth

	// Update both animations
	double runTime = fmod(m_CurrentTime * lowerAnim->m_TicksPerSecond, lowerAnim->m_Duration);
	double aimTime = fmod(m_CurrentTime * upperAnim->m_TicksPerSecond, upperAnim->m_Duration);
	m_CurrentTime += dt;

	std::vector<glm::mat4> runBones(200, glm::mat4(1.0f));
	std::vector<glm::mat4> aimBones(200, glm::mat4(1.0f));

	CalculateBoneTransform(lowerAnim, &lowerAnim->m_RootNode, glm::mat4(1.0f), runBones, runTime);
	CalculateBoneTransform(upperAnim, &upperAnim->m_RootNode, glm::mat4(1.0f), aimBones, aimTime);

	// Mix upper/lower bones
	for (auto& [boneName, info] : lowerAnim->m_BoneInfoMap)
	{
		int id = info.id;
		float mask = 0.0f;
		if (auto it = upperBodyMask.find(boneName); it != upperBodyMask.end())
			mask = it->second * upperBodyBlend;

		float lowerW = 1.0f - mask;
		float upperW = mask;

		m_FinalBoneMatrices[id] = runBones[id] * lowerW + aimBones[id] * upperW;
	}
}

void EasyAnimator::UpdateAnimation(double dt)
{
    if (!m_CurrentAnimation) return;

    m_CurrentTime += m_CurrentAnimation->m_TicksPerSecond * dt;
    m_CurrentTime = fmod(m_CurrentTime, m_CurrentAnimation->m_Duration);

    // Base animation
    std::vector<glm::mat4> baseBones(200, glm::mat4(1.0f));
    CalculateBoneTransform(m_CurrentAnimation, &m_CurrentAnimation->m_RootNode, glm::mat4(1.0f), baseBones);

    if (m_IsBlending && m_NextAnimation)
    {
        m_BlendTime += dt;
        double alpha = std::min(m_BlendTime / m_BlendDuration, 1.0);
        double nextTime = fmod(m_CurrentTime, m_NextAnimation->m_Duration);

        std::vector<glm::mat4> nextBones(200, glm::mat4(1.0f));
        CalculateBoneTransform(m_NextAnimation, &m_NextAnimation->m_RootNode, glm::mat4(1.0f), nextBones, nextTime);

        for (size_t i = 0; i < m_FinalBoneMatrices.size(); ++i)
            m_FinalBoneMatrices[i] = baseBones[i] * (1.0f - (float)alpha) + nextBones[i] * (float)alpha;

        if (alpha >= 1.0)
        {
            m_CurrentAnimation = m_NextAnimation;
            m_NextAnimation = nullptr;
            m_IsBlending = false;
        }
    }
    else
    {
        m_FinalBoneMatrices = baseBones;
    }
}

void EasyAnimator::CalculateBoneTransform(EasyAnimation* animation, const AssimpNodeData* node,const glm::mat4& parentTransform, std::vector<glm::mat4>& outMatrices, double customTime)
{
    double time = (customTime >= 0.0) ? customTime : m_CurrentTime;

    EasyBone* Bone = nullptr;
    if (auto it = animation->boneLookup.find(node->name);
        it != animation->boneLookup.end())
        Bone = it->second;

    glm::mat4 nodeTransform = node->transformation;
    if (Bone)
    {
        Bone->Update(time);
        glm::vec3 pos;
        glm::quat rot;
        glm::vec3 scale;
        glm::vec3 skew;
        glm::vec4 perspective;

        glm::decompose(
            Bone->GetLocalTransform(),
            scale, rot, pos, skew, perspective
        );

        if (mirror)
        {
            // X ekseninde mirror
            pos.x = -pos.x;

            // Quaternion mirror (Y-axis reflection)
            rot.y = -rot.y;
            rot.z = -rot.z;
        }

        nodeTransform =
            glm::translate(glm::mat4(1.0f), pos) *
            glm::toMat4(rot) *
            glm::scale(glm::mat4(1.0f), scale);
    }

    glm::mat4 globalTransform = parentTransform * nodeTransform;
    auto& boneInfoMap = animation->m_BoneInfoMap;
    if (auto it = boneInfoMap.find(node->name); it != boneInfoMap.end())
        outMatrices[it->second.id] = globalTransform * it->second.offset;

    for (int i = 0; i < node->childrenCount; ++i)
        CalculateBoneTransform(animation, &node->children[i], globalTransform, outMatrices, time);
}
