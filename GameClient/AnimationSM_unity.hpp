#pragma once
/*
    AnimationSM (Unity-like Animator Controller)
    -------------------------------------------
    Goals:
    - No dependency on Player / input / rendering.
    - Parameters drive transitions (bool/int/float/trigger).
    - One clip per state.
    - Optional cross-fade via EasyAnimator::BlendTo().
    - Data-driven: build controller in code (later: JSON/ImGui editor).

    Notes:
    - Exit-time / normalized-time based transitions are intentionally optional.
      If you want them, provide a normalized-time provider via SetNormalizedTimeProvider().
*/

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
    enum class ParamType : uint8_t { Bool, Int, Float, Trigger };
    enum class CompareOp : uint8_t
    {
        // For float/int
        Greater,
        GreaterEqual,
        Less,
        LessEqual,
        Equal,
        NotEqual,

        // For bool/trigger convenience
        IsTrue,
        IsFalse
    };

    struct Param
    {
        ParamType type = ParamType::Float;
        bool b = false;
        int  i = 0;
        float f = 0.0f;

        // triggers behave like bool that auto-resets after being consumed
        bool triggerLatched = false;
    };

    struct Condition
    {
        std::string paramName;
        CompareOp op = CompareOp::Greater;
        // value used for float/int compares
        float compareValue = 0.0f;
    };

    struct State
    {
        std::string name;
        EasyAnimation* clip = nullptr;

        bool loop = true;
        float speed = 1.0f;

        // Optional hooks
        std::function<void()> onEnter;
        std::function<void()> onExit;
    };

    struct Transition
    {
        // from = -1 means Any State
        int32_t fromState = -1;
        int32_t toState = -1;

        std::vector<Condition> conditions;

        // Cross-fade duration (seconds). 0 => hard switch.
        float blendDuration = 0.15f;

        // If true, consumes triggers used in conditions when transition fires.
        bool consumeTriggers = true;

        // Optional exit-time gate (Unity-ish). Requires normalized time provider.
        bool hasExitTime = false;
        float exitTime = 0.0f; // normalized [0..1], commonly 0.7 etc.
    };

public:
    AnimationSM() = default;
    explicit AnimationSM(EasyAnimator* animator);

    // Bind / unbind
    void SetAnimator(EasyAnimator* animator);

    // Optional: supply normalized time of CURRENT state (0..1).
    // If not set, exit-time transitions will never pass.
    void SetNormalizedTimeProvider(std::function<float()> fn);

    // --- Parameters (Unity-like) ---
    void AddBool(const std::string& name, bool initial = false);
    void AddInt(const std::string& name, int initial = 0);
    void AddFloat(const std::string& name, float initial = 0.0f);
    void AddTrigger(const std::string& name);

    void SetBool(const std::string& name, bool v);
    void SetInt(const std::string& name, int v);
    void SetFloat(const std::string& name, float v);
    void SetTrigger(const std::string& name);
    void ResetTrigger(const std::string& name);

    bool  GetBool(const std::string& name) const;
    int   GetInt(const std::string& name) const;
    float GetFloat(const std::string& name) const;

    bool HasParam(const std::string& name) const;

    // --- States / transitions ---
    int32_t AddState(const std::string& name, EasyAnimation* clip, bool loop = true, float speed = 1.0f);
    bool    HasState(const std::string& name) const;
    int32_t FindState(const std::string& name) const;

    // Set the default state (called on Start()).
    void SetDefaultState(const std::string& name);
    void SetDefaultState(int32_t stateId);

    // Add transition between named states
    int32_t AddTransition(const std::string& from, const std::string& to, float blendDuration = 0.15f);
    int32_t AddAnyTransition(const std::string& to, float blendDuration = 0.15f);

    // Add condition to a transition by id
    void AddCondition(int32_t transitionId, const Condition& c);
    void AddCondition_Float(int32_t transitionId, const std::string& param, CompareOp op, float v);
    void AddCondition_Int(int32_t transitionId, const std::string& param, CompareOp op, int v);
    void AddCondition_Bool(int32_t transitionId, const std::string& param, bool requiredValue = true);
    void AddCondition_Trigger(int32_t transitionId, const std::string& triggerName);

    // Configure exit time
    void SetExitTime(int32_t transitionId, float normalizedExitTime);

    // --- Runtime ---
    void Start();              // plays default state
    void Stop();               // no-op (kept for symmetry)
    void Update(double dt);    // evaluate transitions and drive animator

    // Debug / query
    int32_t GetCurrentStateId() const { return m_CurrentState; }
    const State* GetCurrentState() const;

private:
    // internal
    bool EvaluateCondition(const Condition& c, std::vector<std::string>* triggersUsed) const;
    bool TransitionPasses(const Transition& t, std::vector<std::string>* triggersUsed) const;
    void ConsumeTriggers(const std::vector<std::string>& triggersUsed);

    void EnterState(int32_t id, bool allowBlend, float blendDuration);
    void ExitState(int32_t id);

    EasyAnimation* GetStateClip(int32_t id) const;

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
