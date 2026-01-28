// EasyAnimator.hpp
#pragma once

#include <vector>
#include <unordered_map>
#include <string>
#include <glm/glm.hpp>

struct AssimpNodeData;
struct EasyBoneInfo;
class EasyAnimation;
class EasyModel;

using BoneMask = std::unordered_map<int, float>;

class EasyAnimator
{
public:
    struct AnimationLayer
    {
        EasyAnimation* clip = nullptr;
        BoneMask mask;
        float weight = 0.0f;
        float m_OverlayTimeSec = 0.0f;

        float mirrorWeight = 0.0f;   // 0 = normal, 1 = mirrored
        float mirrorTarget = 0.0f;
        float mirrorBlendSpeed = 32.0f;

        float targetWeight = 0.0f;
        float blendSpeed = 6.0f; // higher = faster
    };

    struct PlaybackState
    {
        EasyAnimation* clip = nullptr;
        float timeSec = 0.0f;
        bool loop = true;
    };

    struct BlendState
    {
        EasyAnimation* from = nullptr;
        EasyAnimation* to = nullptr;
        float duration = 0.0f;
        float t = 0.0f;
        bool loopTo = true;
    };

    PlaybackState m_Playback;
    BlendState m_Blend;

    bool m_IsBlending = false;
    float m_PlaybackSpeed = 1.0f;

    std::vector<glm::mat4> m_FinalBoneMatrices;
    std::vector<glm::mat4> m_PoseA;
    std::vector<glm::mat4> m_PoseB;
    std::vector<glm::mat4> m_PoseOverlay;

    std::vector<glm::mat4> m_GlobalA;
    std::vector<glm::mat4> m_GlobalB;
    std::vector<glm::mat4> m_GlobalOverlay;
    std::vector<glm::mat4> m_GlobalFinal;
    std::vector<glm::mat4> m_BoneOffsets;
    float m_BlendToTimeSec = 0.0f;

    bool m_HasOverlay = false;
    AnimationLayer m_Overlay;

public:
    EasyAnimator();

    void Update(double dt);

    void PlayAnimation(EasyAnimation* clip, bool loop);
    void BlendTo(EasyAnimation* clip, float duration, bool loop);

    float GetNormalizedTime() const;
    void SetPlaybackSpeed(float speed);

    void SetOverlayLayer(const AnimationLayer& layer);
    void ClearOverlayLayer();

    bool HasOverlay() const;
    void SetOverlayTargetWeight(float w);
    void SetOverlayTargetMirror(float m);

    const std::vector<glm::mat4>& GetFinalBoneMatrices() const;

    static BoneMask upperBodyMask(EasyModel* model);

private:
    void AdvanceTime(double dt);
    void UpdateBlend(double dt);
    void EvaluatePose();

    static void CalculateBoneTransform(EasyAnimation* animation, const AssimpNodeData* node, const glm::mat4& parentTransform, std::vector<glm::mat4>& outMatrices, float timeTicks, float mirror);

    static glm::mat4 BlendMat4(const glm::mat4& a, const glm::mat4& b, float alpha);
    static float MaskWeight(const BoneMask& mask, int id);
    
};
