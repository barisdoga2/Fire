#include "AnimationSM_unity.hpp"

#include <algorithm>
#include <EasyAnimator.hpp>
#include <EasyAnimation.hpp>

AnimationSM::AnimationSM(EasyAnimator* animator)
    : m_Animator(animator)
{
    m_NormalizedTimeProvider = [this]()
    {
        if (m_Animator)
        {
            return m_Animator->GetNormalizedTime();
        }
        return 0.0f;
    };
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

void AnimationSM::SetBool(const std::string& name, bool v)
{
    m_Params[name].b = v;
}

void AnimationSM::SetInt(const std::string& name, int v)
{
    m_Params[name].i = v;
}

void AnimationSM::SetFloat(const std::string& name, float v)
{
    m_Params[name].f = v;
}

void AnimationSM::SetTrigger(const std::string& name)
{
    m_Params[name].triggerLatched = true;
}

void AnimationSM::ResetTrigger(const std::string& name)
{
    auto it = m_Params.find(name);
    if (it != m_Params.end())
    {
        it->second.triggerLatched = false;
    }
}

bool AnimationSM::GetBool(const std::string& name) const
{
    auto it = m_Params.find(name);
    if (it == m_Params.end())
    {
        return false;
    }
    return it->second.b;
}

int AnimationSM::GetInt(const std::string& name) const
{
    auto it = m_Params.find(name);
    if (it == m_Params.end())
    {
        return 0;
    }
    return it->second.i;
}

float AnimationSM::GetFloat(const std::string& name) const
{
    auto it = m_Params.find(name);
    if (it == m_Params.end())
    {
        return 0.0f;
    }
    return it->second.f;
}

int32_t AnimationSM::AddState(const std::string& name, EasyAnimation* clip, bool loop, float speed)
{
    auto it = m_StateNameToId.find(name);
    if (it != m_StateNameToId.end())
    {
        return it->second;
    }

    State s;
    s.name = name;
    s.clip = clip;
    s.loop = loop;
    s.speed = speed;

    int32_t id = (int32_t)m_States.size();
    m_States.push_back(s);
    m_StateNameToId[name] = id;

    if (m_DefaultState < 0)
    {
        m_DefaultState = id;
    }

    return id;
}

int32_t AnimationSM::FindState(const std::string& name) const
{
    auto it = m_StateNameToId.find(name);
    if (it == m_StateNameToId.end())
    {
        return -1;
    }
    return it->second;
}

void AnimationSM::SetDefaultState(const std::string& name)
{
    m_DefaultState = FindState(name);
}

int32_t AnimationSM::AddTransition(const std::string& from, const std::string& to, float blendDuration)
{
    Transition t;
    t.fromState = FindState(from);
    t.toState = FindState(to);
    t.blendDuration = blendDuration;

    int32_t id = (int32_t)m_Transitions.size();
    m_Transitions.push_back(t);
    return id;
}

int32_t AnimationSM::AddAnyTransition(const std::string& to, float blendDuration)
{
    Transition t;
    t.fromState = -1;
    t.toState = FindState(to);
    t.blendDuration = blendDuration;

    int32_t id = (int32_t)m_Transitions.size();
    m_Transitions.push_back(t);
    return id;
}

void AnimationSM::SetCondition(int32_t transitionId, const Condition& c)
{
    if (transitionId < 0 || transitionId >= (int32_t)m_Transitions.size())
    {
        return;
    }

    Transition& t = m_Transitions[transitionId];
    t.condition = c;
    t.hasCondition = true;
}

void AnimationSM::SetExitTime(int32_t transitionId, float normalizedExitTime)
{
    if (transitionId < 0 || transitionId >= (int32_t)m_Transitions.size())
    {
        return;
    }

    Transition& t = m_Transitions[transitionId];
    t.hasExitTime = true;
    t.exitTime = std::clamp(normalizedExitTime, 0.0f, 1.0f);
}

void AnimationSM::Start()
{
    if (m_Started)
    {
        return;
    }

    m_Started = true;

    if (m_DefaultState >= 0)
    {
        EnterState(m_DefaultState, false, 0.0f);
    }
}

void AnimationSM::Update(float dt)
{
    if (!m_Started || m_CurrentState < 0)
    {
        return;
    }

    const float normTime = m_NormalizedTimeProvider ? m_NormalizedTimeProvider() : 0.0f;

    for (Transition& t : m_Transitions)
    {
        if (t.fromState != -1 && t.fromState != m_CurrentState)
        {
            continue;
        }

        if (t.hasExitTime && normTime < t.exitTime)
        {
            continue;
        }

        if (t.toState == m_CurrentState)
        {
            continue;
        }

        if (t.hasCondition && !EvaluateCondition(t.condition))
        {
            continue;
        }

        ExitState(m_CurrentState);
        EnterState(t.toState, true, t.blendDuration);

        if (t.consumeTrigger)
        {
            auto it = m_Params.find(t.condition.paramName);
            if (it != m_Params.end() && it->second.type == ParamType::Trigger)
            {
                it->second.triggerLatched = false;
            }
        }
        break;
    }
}

bool AnimationSM::EvaluateCondition(const Condition& c) const
{
    auto it = m_Params.find(c.paramName);
    if (it == m_Params.end())
    {
        return false;
    }

    const Param& p = it->second;

    switch (c.op)
    {
        case CompareOp::IsTrue: return p.type == ParamType::Trigger ? p.triggerLatched : p.b;
        case CompareOp::IsFalse: return !p.b;
        case CompareOp::Greater: return p.f > c.compareValue;
        case CompareOp::GreaterEqual: return p.f >= c.compareValue;
        case CompareOp::Less: return p.f < c.compareValue;
        case CompareOp::LessEqual: return p.f <= c.compareValue;
        case CompareOp::Equal: return p.f == c.compareValue;
        case CompareOp::NotEqual: return p.f != c.compareValue;
        default: return false;
    }
}

void AnimationSM::EnterState(int32_t id, bool allowBlend, float blendDuration)
{
    State& s = m_States[id];

    if (m_Animator)
    {
        if (allowBlend)
        {
            m_Animator->BlendTo(s.clip, blendDuration, s.loop);
        }
        else
        {
            m_Animator->PlayAnimation(s.clip, s.loop);
        }
        m_Animator->SetPlaybackSpeed(s.speed);
    }

    if (s.onEnter)
    {
        s.onEnter();
    }

    m_CurrentState = id;
}

void AnimationSM::ExitState(int32_t id)
{
    State& s = m_States[id];
    if (s.onExit)
    {
        s.onExit();
    }
}
