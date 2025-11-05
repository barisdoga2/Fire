#pragma once

#include <iostream>
#include <string>
#include <functional>
#include <chrono>
#include <mutex>
#include <thread>
#include <vector>

using namespace std::chrono;


class EasyScheduler {
public:
	class EasySchedulerEntity {
	public:
		uint8_t id;
		std::string name;
		milliseconds period;
		milliseconds offset;
		steady_clock::time_point nextActivation;
		std::atomic<bool> done;
		std::function<void(std::atomic<bool>&)> func;
		std::thread* thread;

	};

	bool running;
	std::thread run;
	std::vector<EasySchedulerEntity*> tasks;
	steady_clock::time_point start;

	EasyScheduler() : running(false)
	{
		
	}

	void AddTask(std::string name, milliseconds period, milliseconds offset, std::function<void(std::atomic<bool>&)> task)
	{
		static uint8_t id_ctr = 0U;

		EasySchedulerEntity* entity = new EasySchedulerEntity();;
		entity->id = ++id_ctr;
		entity->name = name;
		entity->period = period;
		entity->offset = offset;
		entity->thread = nullptr;
		entity->done.store(true);
		entity->func = task;

		tasks.emplace_back(entity);
	}

	void Run()
	{
		start = high_resolution_clock::now();
		for (auto& task : tasks)
			task->nextActivation = start + task->period + task->offset;

		steady_clock::time_point now = start;
		while (running)
		{
			for (auto& task : tasks)
			{
				if (task->thread)
				{
					if (task->done.load())
					{
						task->thread->join();
						delete task->thread;
						task->thread = nullptr;
					}
				}
				else
				{
					if (now <= task->nextActivation)
					{
						task->done.store(false);
						task->thread = new std::thread(task->func, std::ref(task->done));
						task->nextActivation += task->period;
					}
				}
			}
			now = high_resolution_clock::now();
		}
	}

	void Start()
	{
		if (running)
			return;

		running = true;
		run = std::thread(&EasyScheduler::Run, this);
	}

	void Stop()
	{
		if (!running)
			return;

		running = false;

		run.join();
	}

};

//class EasySchedTester {
//public:
//	EasyScheduler sched;
//
//	EasySchedTester()
//	{
//		sched.AddTask("test_Period:10ms_Offset:3ms", milliseconds(10), milliseconds(3), &FunA_10ms_3ms);
//		sched.AddTask("test_Period:10ms_Offset:3ms", milliseconds(10), milliseconds(0), &FunB_10ms_0ms);
//		sched.AddTask("test_Period:10ms_Offset:3ms", milliseconds(10), milliseconds(7), &FunC_10ms_7ms);
//		sched.AddTask("test_Period:10ms_Offset:3ms", milliseconds(20), milliseconds(0), [this](std::atomic<bool>& done) { this->FunD_20ms_0ms(done); });
//
//		sched.Start();
//
//		auto start = high_resolution_clock::now();
//		while (start + milliseconds(10000U) >= high_resolution_clock::now())
//		{}
//
//		sched.Stop();
//	}
//
//	static void FunA_10ms_3ms(std::atomic<bool>& done)
//	{
//		std::cout << "FunA\tPeriod: 10ms\tOffset: 3ms\tCurrent Ts: " << high_resolution_clock::now().time_since_epoch().count() << std::endl;
//		done.store(true);
//	}
//
//	static void FunB_10ms_0ms(std::atomic<bool>& done)
//	{
//		std::cout << "FunB\tPeriod: 10ms\tOffset: 0ms\tCurrent Ts: " << high_resolution_clock::now().time_since_epoch().count() << std::endl;
//		done.store(true);
//	}
//
//	static void FunC_10ms_7ms(std::atomic<bool>& done)
//	{
//		std::cout << "FunC\tPeriod: 10ms\tOffset: 7ms\tCurrent Ts: " << high_resolution_clock::now().time_since_epoch().count() << std::endl;
//		done.store(true);
//	}
//
//	static void FunD_20ms_0ms(std::atomic<bool>& done)
//	{
//		std::cout << "FunD\tPeriod: 20ms\tOffset: 0ms\tCurrent Ts: " << high_resolution_clock::now().time_since_epoch().count() << std::endl;
//		done.store(true);
//	}
//};
