#pragma once

#include <vector>
#include <glm/glm.hpp>



struct AssimpNodeData;
class EasyAnimation;
class EasyAnimator {
private:
    EasyAnimation* m_CurrentAnimation = nullptr;

    std::vector<glm::mat4x4> m_FinalBoneMatrices;
    double m_CurrentTime;

    EasyAnimation* m_NextAnimation = nullptr;
    double m_BlendTime = 0.0;
    double m_BlendDuration = 0.0;
    bool m_IsBlending = false;

    float upperBodyBlend = 0.0f;

    bool mirror = false;

    std::vector<EasyAnimation*> animations{};

public:
    EasyAnimator(const std::vector<EasyAnimation*>& animations);

    void PlayAnimation(EasyAnimation* pAnimation);
    void PlayAnimation(uint8_t aID);
    void BlendTo(EasyAnimation* next, double duration);

    void SetMirror(bool m) { mirror = m; }
    void UpdateAnimation(double dt);
    void UpdateLayered(EasyAnimation* lower, EasyAnimation* upper, bool aiming, double dt);

    const std::vector<glm::mat4>& GetFinalBoneMatrices() const { return m_FinalBoneMatrices; }

private:
    void CalculateBoneTransform(EasyAnimation* anim, const AssimpNodeData* node,
        const glm::mat4& parentTransform, std::vector<glm::mat4>& out,
        double customTime = -1.0);
};
