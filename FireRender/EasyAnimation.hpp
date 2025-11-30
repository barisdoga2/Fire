#pragma once

#include <vector>
#include <string>
#include <map>
#include <unordered_map>

#include <glm/glm.hpp>

#include "EasyUtility.hpp"



struct aiNode;
struct aiNodeAnim;
struct aiScene;
struct aiAnimation;
class EasyBone;
class EasyAnimation {
public:
    double m_Duration;
    double m_TicksPerSecond;
    std::vector<EasyBone*> m_Bones;
    AssimpNodeData m_RootNode;
    std::map<std::string, EasyBoneInfo> m_BoneInfoMap;
    std::unordered_map<std::string, EasyBone*> boneLookup;

    EasyAnimation(const aiScene* scene, const aiAnimation* animation,
        std::map<std::string, EasyBoneInfo>& boneInfoMap, int& boneCount);

private:
    void ReadMissingBones(const aiAnimation* animation,
        std::map<std::string, EasyBoneInfo>& boneInfoMap, int& boneCount);
    void ReadHeirarchyData(AssimpNodeData& dest, const aiNode* src);
};

class EasyBone {
private:
    std::vector<KeyPosition> m_Positions;
    std::vector<KeyRotation> m_Rotations;
    std::vector<KeyScale> m_Scales;
    int m_NumPositions;
    int m_NumRotations;
    int m_NumScalings;

    glm::mat4 m_LocalTransform;
    std::string m_Name;
    int m_ID;


public:
    EasyBone(const std::string& name, int ID, const aiNodeAnim* channel);
    void Update(double animationTime);

    glm::mat4 GetLocalTransform() const { return m_LocalTransform; }
    std::string GetBoneName() const { return m_Name; }
    int GetBoneID() const { return m_ID; }

private:
    int GetPositionIndex(double animationTime) const;
    int GetRotationIndex(double animationTime) const;
    int GetScaleIndex(double animationTime) const;
    double GetScaleFactor(double lastTime, double nextTime, double currentTime) const;

    glm::mat4 InterpolatePosition(double time) const;
    glm::mat4 InterpolateRotation(double time) const;
    glm::mat4 InterpolateScaling(double time) const;
};
