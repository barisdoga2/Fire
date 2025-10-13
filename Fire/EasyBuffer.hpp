#pragma once

#include <iostream>
#include <vector>

class EasyBuffer {
private:
	size_t m_capacity;
	void* m_data;
	EasyBuffer(size_t m_capacity = 256U);

	~EasyBuffer();

public:
	void Release();

	void* Data();

	size_t Capacity();

private:
	inline static std::vector<EasyBuffer*> freeBuffers, busyBuffers;
	inline static bool isInit = false;

public:
	static void Init(size_t buffer_count = 256U, size_t buffer_size = 256U);

	static void DeInit();

	static EasyBuffer* Get();

private:
	static void Free(EasyBuffer* b);

};