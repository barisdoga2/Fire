#pragma once

#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <functional>
#include <atomic>
#include <mutex>

using TickType = uint64_t;
using TaskID = uint32_t;

struct TaskControlBlock {
    TaskID id;
    std::string name;
    std::function<void()> func;
    TickType period;        // in ms
    TickType nextActivation;
    bool ready;
};

class EasyScheduler {
private:
    std::vector<TaskControlBlock> tasks;
    std::atomic<bool> running{ false };
    std::mutex mtx;
    TickType tickMs{ 1U };

public:
    explicit EasyScheduler(TickType tick = 1U) : tickMs(tick) {}

    void AddTask(TaskID id, const std::string& name, TickType period, std::function<void()> func) {
        std::lock_guard<std::mutex> lock(mtx);
        tasks.push_back({ id, name, func, period, GetTick() + period, false });
    }

    void Start() {
        running = true;
        while (running) {
            TickType now = GetTick();
            {
                std::lock_guard<std::mutex> lock(mtx);
                for (auto& t : tasks) {
                    if (now >= t.nextActivation) {
                        t.ready = true;
                        t.nextActivation += t.period;
                    }
                    if (t.ready) {
                        t.ready = false;
                        t.func();
                    }
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(tickMs));
        }
    }

    void Stop() { running = false; }

    static TickType GetTick() {
        using namespace std::chrono;
        return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
    }
};
