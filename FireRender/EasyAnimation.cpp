#include "pch.h"
#include "EasyAnimation.hpp"

#include <cassert>
#include <algorithm>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

#include <assimp/scene.h>

static inline std::vector<KeyPosition> MakePosVec(const aiNodeAnim* channel)
{
    assert(channel);
    std::vector<KeyPosition> out;
    out.reserve(channel->mNumPositionKeys);
    for (unsigned int i = 0; i < channel->mNumPositionKeys; ++i)
    {
        const aiVector3D aiPosition = channel->mPositionKeys[i].mValue;
        const float timeStamp = (float)channel->mPositionKeys[i].mTime;
        out.push_back({ AiToGlm(aiPosition), timeStamp });
    }
    return out;
}

static inline std::vector<KeyRotation> MakeRotVec(const aiNodeAnim* channel)
{
    assert(channel);
    std::vector<KeyRotation> out;
    out.reserve(channel->mNumRotationKeys);
    for (unsigned int i = 0; i < channel->mNumRotationKeys; ++i)
    {
        const aiQuaternion aiOrientation = channel->mRotationKeys[i].mValue;
        const float timeStamp = (float)channel->mRotationKeys[i].mTime;
        out.push_back({ AiToGlm(aiOrientation), timeStamp });
    }
    return out;
}
static inline std::vector<KeyScale> MakeScaVec(const aiNodeAnim* channel)
{
    assert(channel);
    std::vector<KeyScale> out;
    out.reserve(channel->mNumScalingKeys);
    for (unsigned int i = 0; i < channel->mNumScalingKeys; ++i)
    {
        const aiVector3D aiScale = channel->mScalingKeys[i].mValue;
        const float timeStamp = (float)channel->mScalingKeys[i].mTime;
        out.push_back({ AiToGlm(aiScale), timeStamp });
    }
    return out;
}

EasyBone::EasyBone(const std::string& name, int ID, int parentID, const aiNodeAnim* channel) : m_ID(ID), m_ParentID(parentID), m_Name(name), m_Positions(MakePosVec(channel)), m_Rotations(MakeRotVec(channel)), m_Scales(MakeScaVec(channel))
{
    
}

glm::mat4 EasyBone::Evaluate(float animationTimeTicks) const
{
    const glm::mat4 translation = InterpolatePosition(animationTimeTicks);
    const glm::mat4 rotation = InterpolateRotation(animationTimeTicks);
    const glm::mat4 scale = InterpolateScaling(animationTimeTicks);
    return translation * rotation * scale;
}

int EasyBone::GetPositionIndex(float animationTimeTicks) const
{
    const int n = (int)m_Positions.size();
    if (n <= 1)
    {
        return 0;
    }

    for (int i = 0; i < n - 1; ++i)
    {
        if (animationTimeTicks < m_Positions[i + 1].timeStamp)
        {
            return i;
        }
    }

    return n - 2;
}

int EasyBone::GetRotationIndex(float animationTimeTicks) const
{
    const int n = (int)m_Rotations.size();
    if (n <= 1)
    {
        return 0;
    }

    for (int i = 0; i < n - 1; ++i)
    {
        if (animationTimeTicks < m_Rotations[i + 1].timeStamp)
        {
            return i;
        }
    }

    return n - 2;
}

int EasyBone::GetScaleIndex(float animationTimeTicks) const
{
    const int n = (int)m_Scales.size();
    if (n <= 1)
    {
        return 0;
    }

    for (int i = 0; i < n - 1; ++i)
    {
        if (animationTimeTicks < m_Scales[i + 1].timeStamp)
        {
            return i;
        }
    }

    return n - 2;
}

float EasyBone::GetScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTimeTicks)
{
    const float denom = (nextTimeStamp - lastTimeStamp);
    if (denom <= 0.f)
    {
        return 0.f;
    }
    return (animationTimeTicks - lastTimeStamp) / denom;
}

glm::mat4 EasyBone::InterpolatePosition(float animationTimeTicks) const
{
    if (m_Positions.empty())
    {
        return glm::mat4(1.0f);
    }

    if (m_Positions.size() == 1)
    {
        return glm::translate(glm::mat4(1.0f), m_Positions[0].position);
    }

    const int p0 = GetPositionIndex(animationTimeTicks);
    const int p1 = p0 + 1;
    const float t = GetScaleFactor(m_Positions[p0].timeStamp, m_Positions[p1].timeStamp, animationTimeTicks);
    const glm::vec3 pos = glm::mix(m_Positions[p0].position, m_Positions[p1].position, (float)std::clamp(t, 0.f, 1.f));
    return glm::translate(glm::mat4(1.0f), pos);
}

glm::mat4 EasyBone::InterpolateRotation(float animationTimeTicks) const
{
    if (m_Rotations.empty())
    {
        return glm::mat4(1.0f);
    }

    if (m_Rotations.size() == 1)
    {
        return glm::mat4_cast(glm::normalize(m_Rotations[0].orientation));
    }

    const int r0 = GetRotationIndex(animationTimeTicks);
    const int r1 = r0 + 1;
    const float t = GetScaleFactor(m_Rotations[r0].timeStamp, m_Rotations[r1].timeStamp, animationTimeTicks);
    glm::quat q = glm::slerp(m_Rotations[r0].orientation, m_Rotations[r1].orientation, (float)std::clamp(t, 0.f, 1.f));
    q = glm::normalize(q);
    return glm::mat4_cast(q);
}

glm::mat4 EasyBone::InterpolateScaling(float animationTimeTicks) const
{
    if (m_Scales.empty())
    {
        return glm::mat4(1.0f);
    }

    if (m_Scales.size() == 1)
    {
        return glm::scale(glm::mat4(1.0f), m_Scales[0].scale);
    }

    const int s0 = GetScaleIndex(animationTimeTicks);
    const int s1 = s0 + 1;
    const float t = GetScaleFactor(m_Scales[s0].timeStamp, m_Scales[s1].timeStamp, animationTimeTicks);
    const glm::vec3 scl = glm::mix(m_Scales[s0].scale, m_Scales[s1].scale, (float)std::clamp(t, 0.f, 1.f));
    return glm::scale(glm::mat4(1.0f), scl);
}



EasyAnimation::EasyAnimation(const std::string& name, const aiScene* scene, const aiAnimation* animation, std::map<std::string, EasyBoneInfo>& boneInfoMap, int& boneCount, bool loop) : m_Duration((float)animation->mDuration), m_TicksPerSecond(animation->mTicksPerSecond <= 0.f ? 25.f : (float)animation->mTicksPerSecond), m_Name(name), m_DefaultLoop(loop), m_BoneData(CreateData(animation, boneInfoMap, boneCount, scene))
{
    assert(scene);
    assert(scene->mRootNode);
    assert(animation);
}

EasyAnimation::~EasyAnimation()
{
    
}

const EasyBone* EasyAnimation::FindBone(const std::string& name) const
{
    if (const auto it = m_BoneData.m_Bones.find(name); it != m_BoneData.m_Bones.end())
        return &it->second;
    return nullptr;
}

void EasyAnimation::ReadMissingBones(const aiAnimation* animation, std::map<std::string, EasyBoneInfo>& boneInfoMap, std::map<std::string, EasyBone>& bones, int& boneCount)
{
    std::map<std::string, EasyBoneInfo> ret;
    assert(animation);

    const int size = (int)animation->mNumChannels;
    for (int i = 0; i < size; ++i)
    {
        const aiNodeAnim* channel = animation->mChannels[i];
        const std::string boneName = channel->mNodeName.data;

        auto it = boneInfoMap.find(boneName);
        if (it == boneInfoMap.end())
        {
            EasyBoneInfo info{};
            info.id = boneCount++;
            info.parent = -1;
            boneInfoMap.emplace(boneName, info);
            it = boneInfoMap.find(boneName);
        }
        bones.emplace(boneName, EasyBone{ boneName, it->second.id, it->second.parent, channel });
    }
}

EasyAnimation::EasyBoneData EasyAnimation::CreateData(const aiAnimation* animation, std::map<std::string, EasyBoneInfo>& boneInfoMap, int& boneCount, const aiScene* scene)
{
    EasyAnimation::EasyBoneData data;
    ReadHeirarchyData(data.m_RootNode, scene->mRootNode);
    ReadMissingBones(animation, boneInfoMap, data.m_Bones, boneCount);
    data.m_BoneInfoMap = boneInfoMap;
    return data;
}

void EasyAnimation::ReadHeirarchyData(AssimpNodeData& dest, const aiNode* src)
{
    assert(src);

    dest.name = src->mName.data;
    dest.transformation = AiToGlm(src->mTransformation);
    dest.childrenCount = (int)src->mNumChildren;

    dest.children.reserve(src->mNumChildren);
    for (size_t i = 0; i < src->mNumChildren; ++i)
    {
        AssimpNodeData child;
        ReadHeirarchyData(child, src->mChildren[i]);
        dest.children.push_back(std::move(child));
    }
}
