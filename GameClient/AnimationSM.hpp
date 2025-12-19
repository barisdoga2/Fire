#pragma once

#include <vector>
#include <functional>
#include <string>

#include <EasyAnimator.hpp>
#include "Player.hpp"

/*
    Minimal Animation State Machine
    --------------------------------
    - One animation per state
    - Explicit transitions
    - Condition-based switching
*/

class AnimationSM {
public:
    struct State {
        AnimID animation;
        bool loop = true;

        // Optional hooks
        std::function<void(EasyAnimator*)> onEnter{};
        std::function<void(EasyAnimator*)> onExit{};
    };

    struct Transition {
        uint32_t from;
        uint32_t to;
        std::function<bool()> condition;
    };

public:
    explicit AnimationSM(EasyAnimator* animator)
        : animator(animator) {
    }

    // Returns state id
    uint32_t AddState(const State& state)
    {
        states.push_back(state);
        return static_cast<uint32_t>(states.size() - 1U);
    }

    void AddTransition(uint32_t from, uint32_t to,
        std::function<bool()> condition)
    {
        transitions.push_back({ from, to, condition });
    }

    void SetInitialState(uint32_t state)
    {
        current = state;
        EnterState(current);
    }

    void Update(double /*dt*/)
    {
        if (current == INVALID) return;

        // Check transitions
        for (const Transition& t : transitions)
        {
            if (t.from == current && t.condition())
            {
                ExitState(current);
                current = t.to;
                EnterState(current);
                break;
            }
        }
    }

    uint32_t CurrentState() const { return current; }

private:
    static constexpr uint32_t INVALID = 0xFFFFFFFF;

    EasyAnimator* animator{};
    std::vector<State> states{};
    std::vector<Transition> transitions{};
    uint32_t current{ INVALID };

private:
    void EnterState(uint32_t id)
    {
        State& s = states[id];
        if (animator)
            animator->PlayAnimation(s.animation);
        if (s.onEnter)
            s.onEnter(animator);
    }

    void ExitState(uint32_t id)
    {
        State& s = states[id];
        if (s.onExit)
            s.onExit(animator);
    }
};
