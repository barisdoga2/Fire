#pragma once

#include <vector>
#include <string>
#include <map>
#include <unordered_map>

#include <glm/glm.hpp>

#include "EasyUtils.hpp"

struct aiNode;
struct aiNodeAnim;
struct aiScene;
struct aiAnimation;

class EasyBone {
private:
    const std::string m_Name;
    const int m_ID = -1;
    const int m_ParentID = -1;

    const std::vector<KeyPosition> m_Positions;
    const std::vector<KeyRotation> m_Rotations;
    const std::vector<KeyScale> m_Scales;

public:
    EasyBone(const std::string& name, int ID, int parentID, const aiNodeAnim* channel);

    glm::mat4 Evaluate(float animationTimeTicks) const;

    const std::string& Name() const { return m_Name; }
    int ID() const { return m_ID;}
    int ParentID() const{return m_ParentID;}

private:
    int GetPositionIndex(float animationTimeTicks) const;
    int GetRotationIndex(float animationTimeTicks) const;
    int GetScaleIndex(float animationTimeTicks) const;

    static float GetScaleFactor(float lastTime, float nextTime, float currentTime);

    glm::mat4 InterpolatePosition(float timeTicks) const;
    glm::mat4 InterpolateRotation(float timeTicks) const;
    glm::mat4 InterpolateScaling(float timeTicks) const;
};


class EasyAnimation {
private:
    struct EasyBoneData {
        std::map<std::string, EasyBone> m_Bones;
        std::map<std::string, EasyBoneInfo> m_BoneInfoMap;
        AssimpNodeData m_RootNode;
    };

    const std::string m_Name;
    const float m_Duration;
    const float m_TicksPerSecond;
    const bool m_DefaultLoop;

    const EasyBoneData m_BoneData;

public:
    EasyAnimation(const std::string& name, const aiScene* scene, const aiAnimation* animation, std::map<std::string, EasyBoneInfo>& boneInfoMap, int& boneCount, bool loop = true);
    ~EasyAnimation();

    const EasyBone* FindBone(const std::string& name) const;

    const std::string& Name() const { return m_Name; }
    const float Duration() const { return m_Duration; }
    const float TicksPerSecond() const { return m_TicksPerSecond; }
    const bool DefaultLoop() const { return m_DefaultLoop; }
    const AssimpNodeData& RootNode() const { return m_BoneData.m_RootNode; }
    const std::map<std::string, EasyBoneInfo>& BoneInfoMap() const { return m_BoneData.m_BoneInfoMap; }

private:
    static void ReadMissingBones(const aiAnimation* animation, std::map<std::string, EasyBoneInfo>& boneInfoMap, std::map<std::string, EasyBone>& bones, int& boneCount);
    static void ReadHeirarchyData(AssimpNodeData& dest, const aiNode* src);

    static EasyBoneData CreateData(const aiAnimation* animation, std::map<std::string, EasyBoneInfo>& boneInfoMap, int& boneCount, const aiScene* scene);
};
