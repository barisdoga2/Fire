// EasyAnimator.cpp
#include "pch.h"
#include "EasyAnimator.hpp"
#include "EasyAnimation.hpp"
#include "EasyModel.hpp"

#include <algorithm>
#include <cmath>

#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

BoneMask EasyAnimator::upperBodyMask(EasyModel* model)
{
    static bool init{};
    static BoneMask m;
    if (init)
        return m;
    init = true;
    m.emplace(model->GetBoneID("mixamorig:Spine"), 0.3f);
    m.emplace(model->GetBoneID("mixamorig:Spine1"), 0.6f);
    m.emplace(model->GetBoneID("mixamorig:Spine2"), 1.0f);
    m.emplace(model->GetBoneID("mixamorig:Neck"), 1.0f);
    m.emplace(model->GetBoneID("mixamorig:Head"), 1.0f);
    m.emplace(model->GetBoneID("mixamorig:LeftArm"), 1.0f);
    m.emplace(model->GetBoneID("mixamorig:RightArm"), 1.0f);

    m.emplace(model->GetBoneID("mixamorig:Hips"), 1.0f);
    m.emplace(model->GetBoneID("mixamorig:Spine"), 0.8f);
    m.emplace(model->GetBoneID("mixamorig:Spine1"), 0.6f);
    m.emplace(model->GetBoneID("mixamorig:Spine2"), 0.4f);

    m.emplace(model->GetBoneID("mixamorig:Bowl5"), 1.0f);
    m.emplace(model->GetBoneID("mixamorig:Bowl1"), 1.0f);
    m.emplace(model->GetBoneID("mixamorig:Bowl6"), 1.0f);
    m.emplace(model->GetBoneID("mixamorig:Bowl2"), 1.0f);
    m.emplace(model->GetBoneID("mixamorig:Bowl7"), 1.0f);
    m.emplace(model->GetBoneID("mixamorig:Bowl3"), 1.0f);
    m.emplace(model->GetBoneID("mixamorig:Bowl8"), 1.0f);
    m.emplace(model->GetBoneID("mixamorig:Bowl4"), 1.0f);

    m.emplace(model->GetBoneID("mixamorig:Neck"), 1.0f);
    m.emplace(model->GetBoneID("mixamorig:Head"), 1.0f);
    m.emplace(model->GetBoneID("mixamorig:Leye"), 1.0f);
    m.emplace(model->GetBoneID("mixamorig:Reye"), 1.0f);
    m.emplace(model->GetBoneID("mixamorig:Head"), 1.0f);
    m.emplace(model->GetBoneID("mixamorig:HeadTop_End"), 1.0f);
    m.emplace(model->GetBoneID("mixamorig:LeftShoulder"), 1.0f);
    m.emplace(model->GetBoneID("mixamorig:LeftArm"), 1.0f);
    m.emplace(model->GetBoneID("mixamorig:LeftForeArm"), 1.0f);
    m.emplace(model->GetBoneID("mixamorig:LeftHand"), 1.0f);
    m.emplace(model->GetBoneID("mixamorig:RightShoulder"), 1.0f);
    m.emplace(model->GetBoneID("mixamorig:RightArm"), 1.0f);
    m.emplace(model->GetBoneID("mixamorig:RightForeArm"), 1.0f);
    m.emplace(model->GetBoneID("mixamorig:RightHand"), 1.0f);

    m.emplace(model->GetBoneID("mixamorig:RightHandPinky1"), 1.0f);
    m.emplace(model->GetBoneID("mixamorig:RightHandPinky2"), 1.0f);
    m.emplace(model->GetBoneID("mixamorig:RightHandPinky3"), 1.0f);
    m.emplace(model->GetBoneID("mixamorig:RightHandPinky4"), 1.0f);
    m.emplace(model->GetBoneID("mixamorig:RightHandRing1"), 1.0f);
    m.emplace(model->GetBoneID("mixamorig:RightHandRing2"), 1.0f);
    m.emplace(model->GetBoneID("mixamorig:RightHandRing3"), 1.0f);
    m.emplace(model->GetBoneID("mixamorig:RightHandRing4"), 1.0f);
    m.emplace(model->GetBoneID("mixamorig:RightHandMiddle1"), 1.0f);
    m.emplace(model->GetBoneID("mixamorig:RightHandMiddle2"), 1.0f);
    m.emplace(model->GetBoneID("mixamorig:RightHandMiddle3"), 1.0f);
    m.emplace(model->GetBoneID("mixamorig:RightHandMiddle4"), 1.0f);
    m.emplace(model->GetBoneID("mixamorig:RightHandIndex1"), 1.0f);
    m.emplace(model->GetBoneID("mixamorig:RightHandIndex2"), 1.0f);
    m.emplace(model->GetBoneID("mixamorig:RightHandIndex3"), 1.0f);
    m.emplace(model->GetBoneID("mixamorig:RightHandIndex4"), 1.0f);
    m.emplace(model->GetBoneID("mixamorig:RightHandThumb1"), 1.0f);
    m.emplace(model->GetBoneID("mixamorig:RightHandThumb2"), 1.0f);
    m.emplace(model->GetBoneID("mixamorig:RightHandThumb3"), 1.0f);
    m.emplace(model->GetBoneID("mixamorig:RightHandThumb4"), 1.0f);
    m.emplace(model->GetBoneID("mixamorig:LeftHandPinky1"), 1.0f);
    m.emplace(model->GetBoneID("mixamorig:LeftHandPinky2"), 1.0f);
    m.emplace(model->GetBoneID("mixamorig:LeftHandPinky3"), 1.0f);
    m.emplace(model->GetBoneID("mixamorig:LeftHandPinky4"), 1.0f);
    m.emplace(model->GetBoneID("mixamorig:LeftHandRing1"), 1.0f);
    m.emplace(model->GetBoneID("mixamorig:LeftHandRing2"), 1.0f);
    m.emplace(model->GetBoneID("mixamorig:LeftHandRing3"), 1.0f);
    m.emplace(model->GetBoneID("mixamorig:LeftHandRing4"), 1.0f);
    m.emplace(model->GetBoneID("mixamorig:LeftHandMiddle1"), 1.0f);
    m.emplace(model->GetBoneID("mixamorig:LeftHandMiddle2"), 1.0f);
    m.emplace(model->GetBoneID("mixamorig:LeftHandMiddle3"), 1.0f);
    m.emplace(model->GetBoneID("mixamorig:LeftHandMiddle4"), 1.0f);
    m.emplace(model->GetBoneID("mixamorig:LeftHandIndex1"), 1.0f);
    m.emplace(model->GetBoneID("mixamorig:LeftHandIndex2"), 1.0f);
    m.emplace(model->GetBoneID("mixamorig:LeftHandIndex3"), 1.0f);
    m.emplace(model->GetBoneID("mixamorig:LeftHandIndex4"), 1.0f);
    m.emplace(model->GetBoneID("mixamorig:LeftHandThumb1"), 1.0f);
    m.emplace(model->GetBoneID("mixamorig:LeftHandThumb2"), 1.0f);
    m.emplace(model->GetBoneID("mixamorig:LeftHandThumb3"), 1.0f);
    m.emplace(model->GetBoneID("mixamorig:LeftHandThumb4"), 1.0f);
    return m;
};

EasyAnimator::EasyAnimator()
{
    m_Playback = {};
    m_Blend = {};
    m_IsBlending = false;
    m_PlaybackSpeed = 1.0f;

    m_HasOverlay = false;
    m_Overlay = {};
}

void EasyAnimator::SetPlaybackSpeed(float speed)
{
    m_PlaybackSpeed = speed;
}

void EasyAnimator::SetOverlayLayer(const AnimationLayer& layer)
{
    m_Overlay = layer;
    m_HasOverlay = (m_Overlay.clip != nullptr && m_Overlay.weight > 0.0f);
}

void EasyAnimator::ClearOverlayLayer()
{
    m_HasOverlay = false;
    m_Overlay = {};
}

void EasyAnimator::PlayAnimation(EasyAnimation* clip, bool loop)
{
    if (!clip)
    {
        return;
    }

    m_Playback.clip = clip;
    m_Playback.timeSec = 0.0f;
    m_Playback.loop = loop;

    m_IsBlending = false;
    m_Blend = {};
}

void EasyAnimator::BlendTo(EasyAnimation* clip, float duration, bool loop)
{
    if (!clip || duration <= 0.0f || !m_Playback.clip)
    {
        PlayAnimation(clip, loop);
        return;
    }

    if (m_IsBlending && m_Blend.to == clip)
    {
        m_Blend.loopTo = loop;
        return;
    }

    if (m_Playback.clip == clip)
    {
        m_Playback.loop = loop;
        return;
    }

    m_Blend.from = m_Playback.clip;
    m_Blend.to = clip;
    m_Blend.duration = duration;
    m_Blend.t = 0.0f;
    m_Blend.loopTo = loop;

    m_BlendToTimeSec = 0.0f;
    m_IsBlending = true;
    std::cout << m_Blend.to->Name() << "\n";
}


void EasyAnimator::Update(double dt)
{
    AdvanceTime(dt);
    UpdateBlend(dt);
    EvaluatePose();
}

void EasyAnimator::AdvanceTime(double dt)
{
    if (!m_Playback.clip)
        return;

    if (m_HasOverlay)
    {
        const float dtf = (float)dt;

        m_Overlay.m_OverlayTimeSec += dtf * m_PlaybackSpeed;

        m_Overlay.weight = glm::mix(
            m_Overlay.weight,
            m_Overlay.targetWeight,
            glm::clamp(m_Overlay.blendSpeed * dtf, 0.0f, 1.0f)
        );

        m_Overlay.mirrorWeight = glm::mix(
            m_Overlay.mirrorWeight,
            m_Overlay.mirrorTarget,
            glm::clamp(m_Overlay.mirrorBlendSpeed * dtf, 0.0f, 1.0f)
        );
    }


    m_Playback.timeSec += (float)(dt * m_PlaybackSpeed);

    const float durationSec = m_Playback.clip->Duration() / m_Playback.clip->TicksPerSecond();
    if (durationSec <= 0.0f)
        return;

    if (m_Playback.loop)
    {
        while (m_Playback.timeSec >= durationSec)
            m_Playback.timeSec -= durationSec;
    }
    else
    {
        if (m_Playback.timeSec > durationSec)
            m_Playback.timeSec = durationSec;
    }
}

void EasyAnimator::UpdateBlend(double dt)
{
    if (!m_IsBlending)
    {
        return;
    }

    m_Blend.t += (float)(dt * m_PlaybackSpeed);
    m_BlendToTimeSec += (float)(dt * m_PlaybackSpeed);

    if (m_Blend.t < m_Blend.duration)
    {
        return;
    }

    m_Playback.clip = m_Blend.to;
    m_Playback.timeSec = m_BlendToTimeSec;
    m_Playback.loop = m_Blend.loopTo;

    m_Blend = {};
    m_IsBlending = false;
    m_BlendToTimeSec = 0.0f;
}

float EasyAnimator::GetNormalizedTime() const
{
    if (!m_Playback.clip)
    {
        return 0.0f;
    }

    const float durationSec = m_Playback.clip->Duration() / m_Playback.clip->TicksPerSecond();
    if (durationSec <= 0.0f)
    {
        return 0.0f;
    }

    float t = m_Playback.timeSec / durationSec;

    if (m_Playback.loop)
    {
        t = std::fmod(t, 1.0f);
        if (t < 0.0f)
        {
            t += 1.0f;
        }
    }
    else
    {
        t = std::clamp(t, 0.0f, 1.0f);
    }

    return t;
}

bool EasyAnimator::HasOverlay() const
{
    return m_HasOverlay;
}

void EasyAnimator::SetOverlayTargetMirror(float m)
{
    if (m_HasOverlay)
    {
        m_Overlay.mirrorTarget = m;
    }
}

void EasyAnimator::SetOverlayTargetWeight(float w)
{
    m_Overlay.targetWeight = std::clamp(w, 0.0f, 1.0f);

    if (m_Overlay.targetWeight <= 0.0f && m_Overlay.weight <= 0.001f)
    {
        ClearOverlayLayer();
    }
}

const std::vector<glm::mat4>& EasyAnimator::GetFinalBoneMatrices() const
{
    return m_FinalBoneMatrices;
}

float EasyAnimator::MaskWeight(const BoneMask& mask, int id)
{
    auto it = mask.find(id);
    if (it == mask.end())
    {
        return 0.0f;
    }
    return std::clamp(it->second, 0.0f, 1.0f);
}

glm::mat4 EasyAnimator::BlendMat4(const glm::mat4& a, const glm::mat4& b, float alpha)
{
    alpha = std::clamp(alpha, 0.0f, 1.0f);

    glm::vec3 ta, tb, sa, sb, skew;
    glm::vec4 persp;
    glm::quat ra, rb;

    glm::decompose(a, sa, ra, ta, skew, persp);
    glm::decompose(b, sb, rb, tb, skew, persp);

    glm::vec3 t = glm::mix(ta, tb, alpha);
    glm::vec3 s = glm::mix(sa, sb, alpha);
    glm::quat r = glm::normalize(glm::slerp(ra, rb, alpha));

    glm::mat4 M(1.0f);
    M = glm::translate(M, t);
    M *= glm::mat4_cast(r);
    M = glm::scale(M, s);
    return M;
}

void EasyAnimator::EvaluatePose()
{
    if (!m_Playback.clip)
    {
        return;
    }

    const auto& boneInfoMap = m_Playback.clip->BoneInfoMap();
    const size_t boneCount = boneInfoMap.size();

    if (m_FinalBoneMatrices.size() != boneCount)
    {
        m_FinalBoneMatrices.assign(boneCount, glm::mat4(1.0f));
    }

    if (m_GlobalA.size() != boneCount) { m_GlobalA.assign(boneCount, glm::mat4(1.0f)); }
    if (m_GlobalB.size() != boneCount) { m_GlobalB.assign(boneCount, glm::mat4(1.0f)); }
    if (m_GlobalOverlay.size() != boneCount) { m_GlobalOverlay.assign(boneCount, glm::mat4(1.0f)); }
    if (m_GlobalFinal.size() != boneCount) { m_GlobalFinal.assign(boneCount, glm::mat4(1.0f)); }
    if (m_BoneOffsets.size() != boneCount) { m_BoneOffsets.assign(boneCount, glm::mat4(1.0f)); }

    for (auto& kv : boneInfoMap)
    {
        const int id = kv.second.id;
        if (id >= 0 && (size_t)id < boneCount)
        {
            m_BoneOffsets[(size_t)id] = kv.second.offset;
        }
    }

    const float timeTicksA = m_Playback.timeSec * m_Playback.clip->TicksPerSecond();

    if (!m_IsBlending)
    {
        CalculateBoneTransform(m_Playback.clip, &m_Playback.clip->RootNode(), glm::mat4(1.0f), m_GlobalFinal, timeTicksA, 1.f);
    }
    else
    {
        const float alpha = std::clamp(m_Blend.t / m_Blend.duration, 0.0f, 1.0f);
        const float timeTicksB = m_BlendToTimeSec * m_Blend.to->TicksPerSecond();

        CalculateBoneTransform(m_Blend.from, &m_Blend.from->RootNode(), glm::mat4(1.0f), m_GlobalA, timeTicksA, 1.f);
        CalculateBoneTransform(m_Blend.to, &m_Blend.to->RootNode(), glm::mat4(1.0f), m_GlobalB, timeTicksB, 1.f);

        for (size_t i = 0; i < boneCount; ++i)
        {
            m_GlobalFinal[i] = BlendMat4(m_GlobalA[i], m_GlobalB[i], alpha);
        }
    }

    if (m_HasOverlay && m_Overlay.clip && m_Overlay.weight > 0.0f)
    {
        const float timeTicksO = m_Overlay.m_OverlayTimeSec * m_Overlay.clip->TicksPerSecond();
        CalculateBoneTransform(m_Overlay.clip, &m_Overlay.clip->RootNode(), glm::mat4(1.0f), m_GlobalOverlay, timeTicksO, m_Overlay.mirrorWeight);

        for (size_t i = 0; i < boneCount; ++i)
        {
            const float w = MaskWeight(m_Overlay.mask, (int)i) * std::clamp(m_Overlay.weight, 0.0f, 1.0f);
            if (w > 0.0f)
            {
                m_GlobalFinal[i] = BlendMat4(m_GlobalFinal[i], m_GlobalOverlay[i], w);
            }
        }
    }

    for (size_t i = 0; i < boneCount; ++i)
    {
        m_FinalBoneMatrices[i] = m_GlobalFinal[i] * m_BoneOffsets[i];
    }
}

void EasyAnimator::CalculateBoneTransform(EasyAnimation* animation, const AssimpNodeData* node, const glm::mat4& parentTransform, std::vector<glm::mat4>& outGlobals, float timeTicks, float mirror)
{
    if (!animation || !node)
    {
        return;
    }

    const EasyBone* bone = animation->FindBone(node->name);

    glm::mat4 nodeTransform = node->transformation;

    if (bone)
    {
        nodeTransform = bone->Evaluate(timeTicks);
    }
    if (mirror <= 0.0f)
        mirror = -1.f;
    else if (mirror >= 0.0f)
        mirror = 1.f;

    const glm::mat4 MirrorX =
    {
        1 * mirror,  0,  0, 0,
         0,  1 ,  0, 0,
         0,  0,  1 , 0,
         0,  0,  0, 1
    };
    {
         nodeTransform = MirrorX * nodeTransform * MirrorX;
    }

    const glm::mat4 globalTransform = parentTransform * nodeTransform;

    const std::map<std::string, EasyBoneInfo>& boneInfoMap = animation->BoneInfoMap();
    auto it = boneInfoMap.find(node->name);
    if (it != boneInfoMap.end())
    {
        const int id = it->second.id;
        if (id >= 0 && (size_t)id < outGlobals.size())
        {
            outGlobals[(size_t)id] = globalTransform;
        }
    }

    for (int i = 0; i < node->childrenCount; ++i)
    {
        CalculateBoneTransform(animation, &node->children[i], globalTransform, outGlobals, timeTicks, mirror);
    }
}
