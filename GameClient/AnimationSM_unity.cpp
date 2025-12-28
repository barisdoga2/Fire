#include "AnimationSM_unity.hpp"

#include <algorithm>
#include <EasyAnimator.hpp>
#include <EasyAnimation.hpp>

AnimationSM::AnimationSM(EasyAnimator* animator)
    : m_Animator(animator)
{
}

void AnimationSM::SetAnimator(EasyAnimator* animator)
{
    m_Animator = animator;
}

void AnimationSM::SetNormalizedTimeProvider(std::function<float()> fn)
{
    m_NormalizedTimeProvider = std::move(fn);
}

void AnimationSM::AddBool(const std::string& name, bool initial)
{
    Param& p = m_Params[name];
    p.type = ParamType::Bool;
    p.b = initial;
}
void AnimationSM::AddInt(const std::string& name, int initial)
{
    Param& p = m_Params[name];
    p.type = ParamType::Int;
    p.i = initial;
}
void AnimationSM::AddFloat(const std::string& name, float initial)
{
    Param& p = m_Params[name];
    p.type = ParamType::Float;
    p.f = initial;
}
void AnimationSM::AddTrigger(const std::string& name)
{
    Param& p = m_Params[name];
    p.type = ParamType::Trigger;
    p.triggerLatched = false;
}

bool AnimationSM::HasParam(const std::string& name) const
{
    return m_Params.find(name) != m_Params.end();
}

void AnimationSM::SetBool(const std::string& name, bool v)
{
    Param& p = m_Params[name];
    p.type = ParamType::Bool;
    p.b = v;
}
void AnimationSM::SetInt(const std::string& name, int v)
{
    Param& p = m_Params[name];
    p.type = ParamType::Int;
    p.i = v;
}
void AnimationSM::SetFloat(const std::string& name, float v)
{
    Param& p = m_Params[name];
    p.type = ParamType::Float;
    p.f = v;
}
void AnimationSM::SetTrigger(const std::string& name)
{
    Param& p = m_Params[name];
    p.type = ParamType::Trigger;
    p.triggerLatched = true;
}
void AnimationSM::ResetTrigger(const std::string& name)
{
    auto it = m_Params.find(name);
    if (it == m_Params.end()) return;
    if (it->second.type == ParamType::Trigger)
        it->second.triggerLatched = false;
}

bool AnimationSM::GetBool(const std::string& name) const
{
    auto it = m_Params.find(name);
    if (it == m_Params.end()) return false;
    return it->second.b;
}
int AnimationSM::GetInt(const std::string& name) const
{
    auto it = m_Params.find(name);
    if (it == m_Params.end()) return 0;
    return it->second.i;
}
float AnimationSM::GetFloat(const std::string& name) const
{
    auto it = m_Params.find(name);
    if (it == m_Params.end()) return 0.0f;
    return it->second.f;
}

int32_t AnimationSM::AddState(const std::string& name, EasyAnimation* clip, bool loop, float speed)
{
    if (name.empty()) return -1;
    if (m_StateNameToId.find(name) != m_StateNameToId.end())
        return m_StateNameToId[name];

    State s;
    s.name = name;
    s.clip = clip;
    s.loop = loop;
    s.speed = speed;

    int32_t id = (int32_t)m_States.size();
    m_States.push_back(std::move(s));
    m_StateNameToId[name] = id;

    if (m_DefaultState < 0)
        m_DefaultState = id;

    return id;
}

bool AnimationSM::HasState(const std::string& name) const
{
    return m_StateNameToId.find(name) != m_StateNameToId.end();
}

int32_t AnimationSM::FindState(const std::string& name) const
{
    auto it = m_StateNameToId.find(name);
    return (it == m_StateNameToId.end()) ? -1 : it->second;
}

void AnimationSM::SetDefaultState(const std::string& name)
{
    SetDefaultState(FindState(name));
}

void AnimationSM::SetDefaultState(int32_t stateId)
{
    if (stateId < 0 || stateId >= (int32_t)m_States.size()) return;
    m_DefaultState = stateId;
}

int32_t AnimationSM::AddTransition(const std::string& from, const std::string& to, float blendDuration)
{
    int32_t fromId = FindState(from);
    int32_t toId = FindState(to);
    if (fromId < 0 || toId < 0) return -1;

    Transition t;
    t.fromState = fromId;
    t.toState = toId;
    t.blendDuration = blendDuration;

    int32_t id = (int32_t)m_Transitions.size();
    m_Transitions.push_back(std::move(t));
    return id;
}

int32_t AnimationSM::AddAnyTransition(const std::string& to, float blendDuration)
{
    int32_t toId = FindState(to);
    if (toId < 0) return -1;

    Transition t;
    t.fromState = -1;
    t.toState = toId;
    t.blendDuration = blendDuration;

    int32_t id = (int32_t)m_Transitions.size();
    m_Transitions.push_back(std::move(t));
    return id;
}

void AnimationSM::AddCondition(int32_t transitionId, const Condition& c)
{
    if (transitionId < 0 || transitionId >= (int32_t)m_Transitions.size()) return;
    m_Transitions[transitionId].conditions.push_back(c);
}

void AnimationSM::AddCondition_Float(int32_t transitionId, const std::string& param, CompareOp op, float v)
{
    Condition c;
    c.paramName = param;
    c.op = op;
    c.compareValue = v;
    AddCondition(transitionId, c);
}

void AnimationSM::AddCondition_Int(int32_t transitionId, const std::string& param, CompareOp op, int v)
{
    Condition c;
    c.paramName = param;
    c.op = op;
    c.compareValue = (float)v;
    AddCondition(transitionId, c);
}

void AnimationSM::AddCondition_Bool(int32_t transitionId, const std::string& param, bool requiredValue)
{
    Condition c;
    c.paramName = param;
    c.op = requiredValue ? CompareOp::IsTrue : CompareOp::IsFalse;
    c.compareValue = requiredValue ? 1.0f : 0.0f;
    AddCondition(transitionId, c);
}

void AnimationSM::AddCondition_Trigger(int32_t transitionId, const std::string& triggerName)
{
    Condition c;
    c.paramName = triggerName;
    c.op = CompareOp::IsTrue;
    c.compareValue = 1.0f;
    AddCondition(transitionId, c);
}

void AnimationSM::SetExitTime(int32_t transitionId, float normalizedExitTime)
{
    if (transitionId < 0 || transitionId >= (int32_t)m_Transitions.size()) return;
    Transition& t = m_Transitions[transitionId];
    t.hasExitTime = true;
    t.exitTime = std::clamp(normalizedExitTime, 0.0f, 1.0f);
}

void AnimationSM::Start()
{
    if (m_Started) return;
    m_Started = true;

    if (m_DefaultState < 0 && !m_States.empty())
        m_DefaultState = 0;

    if (m_DefaultState >= 0)
        EnterState(m_DefaultState, /*allowBlend*/false, 0.0f);
}

void AnimationSM::Stop()
{
    // intentionally minimal for now
}

void AnimationSM::Update(double /*dt*/)
{
    if (!m_Started)
        Start();

    if (m_CurrentState < 0)
        return;

    // 1) Check Any State transitions first (Unity-like)
    for (int32_t i = 0; i < (int32_t)m_Transitions.size(); ++i)
    {
        const Transition& t = m_Transitions[i];
        if (t.fromState != -1) continue;

        std::vector<std::string> triggersUsed;
        if (TransitionPasses(t, &triggersUsed))
        {
            EnterState(t.toState, /*allowBlend*/true, t.blendDuration);
            if (t.consumeTriggers) ConsumeTriggers(triggersUsed);
            return;
        }
    }

    // 2) Check transitions from current state
    for (int32_t i = 0; i < (int32_t)m_Transitions.size(); ++i)
    {
        const Transition& t = m_Transitions[i];
        if (t.fromState != m_CurrentState) continue;

        std::vector<std::string> triggersUsed;
        if (TransitionPasses(t, &triggersUsed))
        {
            EnterState(t.toState, /*allowBlend*/true, t.blendDuration);
            if (t.consumeTriggers) ConsumeTriggers(triggersUsed);
            return;
        }
    }
}

const AnimationSM::State* AnimationSM::GetCurrentState() const
{
    if (m_CurrentState < 0 || m_CurrentState >= (int32_t)m_States.size()) return nullptr;
    return &m_States[m_CurrentState];
}

bool AnimationSM::EvaluateCondition(const Condition& c, std::vector<std::string>* triggersUsed) const
{
    auto it = m_Params.find(c.paramName);
    if (it == m_Params.end())
        return false;

    const Param& p = it->second;

    auto cmpFloat = [&](float lhs) -> bool
    {
        switch (c.op)
        {
        case CompareOp::Greater:       return lhs >  c.compareValue;
        case CompareOp::GreaterEqual:  return lhs >= c.compareValue;
        case CompareOp::Less:          return lhs <  c.compareValue;
        case CompareOp::LessEqual:     return lhs <= c.compareValue;
        case CompareOp::Equal:         return lhs == c.compareValue;
        case CompareOp::NotEqual:      return lhs != c.compareValue;
        default:                       return false;
        }
    };

    switch (p.type)
    {
    case ParamType::Float:
        return cmpFloat(p.f);

    case ParamType::Int:
        return cmpFloat((float)p.i);

    case ParamType::Bool:
        if (c.op == CompareOp::IsTrue)  return p.b == true;
        if (c.op == CompareOp::IsFalse) return p.b == false;
        // allow == / != with compareValue 0/1
        if (c.op == CompareOp::Equal)   return (p.b ? 1.0f : 0.0f) == c.compareValue;
        if (c.op == CompareOp::NotEqual)return (p.b ? 1.0f : 0.0f) != c.compareValue;
        return false;

    case ParamType::Trigger:
        if (triggersUsed)
            triggersUsed->push_back(c.paramName);
        if (c.op == CompareOp::IsTrue)  return p.triggerLatched == true;
        if (c.op == CompareOp::IsFalse) return p.triggerLatched == false;
        if (c.op == CompareOp::Equal)   return (p.triggerLatched ? 1.0f : 0.0f) == c.compareValue;
        if (c.op == CompareOp::NotEqual)return (p.triggerLatched ? 1.0f : 0.0f) != c.compareValue;
        return false;

    default:
        return false;
    }
}

bool AnimationSM::TransitionPasses(const Transition& t, std::vector<std::string>* triggersUsed) const
{
    // Exit time gate
    if (t.hasExitTime)
    {
        if (!m_NormalizedTimeProvider)
            return false;

        float nt = m_NormalizedTimeProvider();
        if (nt < t.exitTime)
            return false;
    }

    // Conditions (AND)
    for (const Condition& c : t.conditions)
    {
        if (!EvaluateCondition(c, triggersUsed))
            return false;
    }

    return true;
}

void AnimationSM::ConsumeTriggers(const std::vector<std::string>& triggersUsed)
{
    for (const std::string& n : triggersUsed)
        ResetTrigger(n);
}

void AnimationSM::EnterState(int32_t id, bool allowBlend, float blendDuration)
{
    if (id < 0 || id >= (int32_t)m_States.size())
        return;

    if (id == m_CurrentState)
        return;

    if (m_CurrentState >= 0)
        ExitState(m_CurrentState);

    m_CurrentState = id;

    if (blendDuration > 0.0f && allowBlend)
        m_Animator->BlendTo(m_CurrentState, blendDuration);
    else
        m_Animator->PlayAnimation(m_CurrentState);

    if (m_States[id].onEnter)
        m_States[id].onEnter();
}

void AnimationSM::ExitState(int32_t id)
{
    if (id < 0 || id >= (int32_t)m_States.size())
        return;

    if (m_States[id].onExit)
        m_States[id].onExit();
}

EasyAnimation* AnimationSM::GetStateClip(int32_t id) const
{
    if (id < 0 || id >= (int32_t)m_States.size())
        return nullptr;
    return m_States[id].clip;
}
