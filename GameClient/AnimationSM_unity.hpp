#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <functional>

class EasyAnimator;
class EasyAnimation;

class AnimationSM
{
public:
    enum class ParamType : uint8_t
    {
        Bool,
        Int,
        Float,
        Trigger
    };

    enum class CompareOp : uint8_t
    {
        Greater,
        GreaterEqual,
        Less,
        LessEqual,
        Equal,
        NotEqual,
        IsTrue,
        IsFalse
    };

    struct Param
    {
        ParamType type = ParamType::Float;
        bool b = false;
        int i = 0;
        float f = 0.0f;
        bool triggerLatched = false;
    };

    struct Condition
    {
        std::string paramName;
        CompareOp op = CompareOp::Greater;
        float compareValue = 0.0f;
    };

    struct State
    {
        std::string name;
        EasyAnimation* clip = nullptr;
        bool loop = true;
        float speed = 1.0f;
        std::function<void()> onEnter;
        std::function<void()> onExit;
    };

    struct Transition
    {
        int32_t fromState = -1;
        int32_t toState = -1;
        float blendDuration = 0.15f;

        bool hasExitTime = false;
        float exitTime = 1.0f;

        bool consumeTrigger = true;
        Condition condition;
        bool hasCondition = false;
    };

public:
    AnimationSM() = default;
    explicit AnimationSM(EasyAnimator* animator);

    void SetAnimator(EasyAnimator* animator);
    void SetNormalizedTimeProvider(std::function<float()> fn);

    void AddBool(const std::string& name, bool initial = false);
    void AddInt(const std::string& name, int initial = 0);
    void AddFloat(const std::string& name, float initial = 0.0f);
    void AddTrigger(const std::string& name);

    void SetBool(const std::string& name, bool v);
    void SetInt(const std::string& name, int v);
    void SetFloat(const std::string& name, float v);
    void SetTrigger(const std::string& name);
    void ResetTrigger(const std::string& name);

    bool GetBool(const std::string& name) const;
    int GetInt(const std::string& name) const;
    float GetFloat(const std::string& name) const;

    int32_t AddState(const std::string& name, EasyAnimation* clip, bool loop = true, float speed = 1.0f);
    int32_t FindState(const std::string& name) const;

    void SetDefaultState(const std::string& name);

    int32_t AddTransition(const std::string& from, const std::string& to, float blendDuration = 0.15f);
    int32_t AddAnyTransition(const std::string& to, float blendDuration = 0.15f);

    void SetCondition(int32_t transitionId, const Condition& c);
    void SetExitTime(int32_t transitionId, float normalizedExitTime);

    void Start();
    void Update(float dt);

private:
    bool EvaluateCondition(const Condition& c) const;
    void EnterState(int32_t id, bool allowBlend, float blendDuration);
    void ExitState(int32_t id);

private:
    EasyAnimator* m_Animator = nullptr;
    std::function<float()> m_NormalizedTimeProvider;

    std::unordered_map<std::string, Param> m_Params;
    std::vector<State> m_States;
    std::unordered_map<std::string, int32_t> m_StateNameToId;
    std::vector<Transition> m_Transitions;

    int32_t m_DefaultState = -1;
    int32_t m_CurrentState = -1;
    bool m_Started = false;
};
