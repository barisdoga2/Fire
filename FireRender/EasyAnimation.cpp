#include "pch.h"
#include "EasyAnimation.hpp"

#include <cassert>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <assimp/scene.h>



EasyBone::EasyBone(const std::string& name, int ID, int parentID, const aiNodeAnim* channel) : m_Name(name), m_ID(ID), m_ParentID(parentID), m_LocalTransform(1.0f)
{
    m_NumPositions = channel->mNumPositionKeys;

    for (int positionIndex = 0; positionIndex < m_NumPositions; ++positionIndex)
    {
        aiVector3D aiPosition = channel->mPositionKeys[positionIndex].mValue;
        double timeStamp = channel->mPositionKeys[positionIndex].mTime;
        KeyPosition data;
        data.position = AiToGlm(aiPosition);
        data.timeStamp = timeStamp;
        m_Positions.push_back(data);
    }

    m_NumRotations = channel->mNumRotationKeys;
    for (int rotationIndex = 0; rotationIndex < m_NumRotations; ++rotationIndex)
    {
        aiQuaternion aiOrientation = channel->mRotationKeys[rotationIndex].mValue;
        double timeStamp = channel->mRotationKeys[rotationIndex].mTime;
        KeyRotation data;
        data.orientation = AiToGlm(aiOrientation);
        data.timeStamp = timeStamp;
        m_Rotations.push_back(data);
    }

    m_NumScalings = channel->mNumScalingKeys;
    for (int keyIndex = 0; keyIndex < m_NumScalings; ++keyIndex)
    {
        aiVector3D scale = channel->mScalingKeys[keyIndex].mValue;
        double timeStamp = channel->mScalingKeys[keyIndex].mTime;
        KeyScale data;
        data.scale = AiToGlm(scale);
        data.timeStamp = timeStamp;
        m_Scales.push_back(data);
    }
}

void EasyBone::Update(double animationTime)
{
    const glm::mat4 translation = InterpolatePosition(animationTime);
    const glm::mat4 rotation = InterpolateRotation(animationTime);
    const glm::mat4 scale = InterpolateScaling(animationTime);
    m_LocalTransform = translation * rotation * scale;
}

int EasyBone::GetPositionIndex(double animationTime) const
{
    for (int index = 0; index < m_NumPositions - 1; ++index)
    {
        if (animationTime < m_Positions[index + 1].timeStamp)
            return index;
    }
    assert(0);
    return 0;
}

int EasyBone::GetRotationIndex(double animationTime) const
{
    for (int index = 0; index < m_NumRotations - 1; ++index)
    {
        if (animationTime < m_Rotations[index + 1].timeStamp)
            return index;
    }
    assert(0);
    return 0;
}

int EasyBone::GetScaleIndex(double animationTime) const
{
    for (int index = 0; index < m_NumScalings - 1; ++index)
    {
        if (animationTime < m_Scales[index + 1].timeStamp)
            return index;
    }
    assert(0);
    return 0;
}

double EasyBone::GetScaleFactor(double lastTimeStamp, double nextTimeStamp, double animationTime) const
{
    double scaleFactor = 0.0;
    double midWayLength = animationTime - lastTimeStamp;
    double framesDiff = nextTimeStamp - lastTimeStamp;
    scaleFactor = midWayLength / framesDiff;
    return scaleFactor;
}

glm::mat4 EasyBone::InterpolatePosition(double animationTime) const
{
    if (1 == m_NumPositions)
        return glm::translate(glm::mat4(1.0f), m_Positions[0].position);

    int p0Index = GetPositionIndex(animationTime);
    int p1Index = p0Index + 1;
    double scaleFactor = GetScaleFactor(m_Positions[p0Index].timeStamp,
        m_Positions[p1Index].timeStamp, animationTime);
    glm::vec3 finalPosition = glm::mix(m_Positions[p0Index].position,
        m_Positions[p1Index].position, (float)scaleFactor);
    return glm::translate(glm::mat4(1.0f), finalPosition);
}

glm::mat4 EasyBone::InterpolateRotation(double animationTime) const
{
    if (1 == m_NumRotations)
    {
        auto rotation = glm::normalize(m_Rotations[0].orientation);
        return glm::mat4_cast(rotation);
    }

    int p0Index = GetRotationIndex(animationTime);
    int p1Index = p0Index + 1;
    double scaleFactor = GetScaleFactor(m_Rotations[p0Index].timeStamp, m_Rotations[p1Index].timeStamp, animationTime);
    glm::quat finalRotation = glm::slerp(m_Rotations[p0Index].orientation, m_Rotations[p1Index].orientation, (float)scaleFactor);
    finalRotation = glm::normalize(finalRotation);
    return glm::mat4_cast(finalRotation);
}

glm::mat4 EasyBone::InterpolateScaling(double animationTime) const
{
    if (1 == m_NumScalings)
        return glm::scale(glm::mat4(1.0f), m_Scales[0].scale);

    int p0Index = GetScaleIndex(animationTime);
    int p1Index = p0Index + 1;
    double scaleFactor = GetScaleFactor(m_Scales[p0Index].timeStamp, m_Scales[p1Index].timeStamp, animationTime);
    glm::vec3 finalScale = glm::mix(m_Scales[p0Index].scale, m_Scales[p1Index].scale, (float)scaleFactor);
    return glm::scale(glm::mat4(1.0f), finalScale);
}

EasyAnimation::EasyAnimation(const std::string& name, const aiScene* scene, const aiAnimation* animation, std::map<std::string, EasyBoneInfo>& boneInfoMap, int& boneCount)
{
    assert(scene && scene->mRootNode);
    m_Name = name;
    m_Duration = animation->mDuration;
    m_TicksPerSecond = animation->mTicksPerSecond;
    ReadHeirarchyData(m_RootNode, scene->mRootNode);
    ReadMissingBones(animation, boneInfoMap, boneCount);
}

void EasyAnimation::ReadMissingBones(const aiAnimation* animation, std::map<std::string, EasyBoneInfo>& boneInfoMap, int& boneCount)
{
    int size = animation->mNumChannels;

    for (int i = 0; i < size; i++)
    {
        auto channel = animation->mChannels[i];
        std::string boneName = channel->mNodeName.data;

        auto it = boneInfoMap.find(boneName);
        if (it == boneInfoMap.end())
        {
            EasyBoneInfo info{};
            info.id = boneCount++;
            info.parent = -1;
            boneInfoMap.emplace(boneName, info);
            it = boneInfoMap.find(boneName);
        }

        EasyBone* bone = new EasyBone(boneName, it->second.id, it->second.parent, channel);
        m_Bones.push_back(bone);
        boneLookup[boneName] = bone;
    }

    m_BoneInfoMap = boneInfoMap;
}

void EasyAnimation::ReadHeirarchyData(AssimpNodeData& dest, const aiNode* src)
{
    assert(src);

    dest.name = src->mName.data;
    dest.transformation = AiToGlm(src->mTransformation);
    dest.childrenCount = src->mNumChildren;

    for (size_t i = 0; i < src->mNumChildren; i++)
    {
        AssimpNodeData newData;
        ReadHeirarchyData(newData, src->mChildren[i]);
        dest.children.push_back(newData);
    }
}
