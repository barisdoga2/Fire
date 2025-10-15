#pragma once

#include <iostream>
#include <vector>

class EasyBuffer : std::vector<uint8_t> {
private:
	EasyBuffer();

	~EasyBuffer();

public:
	size_t ptr;

	void Release();

	void* Data();

	size_t Capacity();

private:
	inline static std::vector<EasyBuffer*> freeBuffers, busyBuffers;
	inline static bool isInit = false;

public:
	static void Init(size_t buffer_count = 256U);

	static void DeInit();

	static EasyBuffer* Get();

private:
	static void Free(EasyBuffer* b);

};

class BufferType {
public:
	std::vector<uint8_t> buff;
	size_t ptr;
	BufferType(size_t capacity) : buff(capacity), ptr(0U) {}
	uint8_t* data() { return buff.data(); }
	size_t size() { return buff.size(); }
};

class EasyBufferManager {
public:
	using BufferPool = std::vector<BufferType>;
	using BufferPtrPool = std::vector<BufferType*>;
	const size_t buffer_count, buffer_length;
	BufferPool buffers;
	BufferPtrPool free_buffers;
	BufferPtrPool busy_buffers;

	EasyBufferManager() = delete;

	EasyBufferManager(const size_t buffer_count, const size_t buffer_length) : buffer_count(buffer_count), buffer_length(buffer_length), buffers(buffer_count, BufferType(buffer_length))
	{
		for (size_t i = 0U ; i < buffer_count ; ++i)
			free_buffers.push_back(&buffers[i]);
	}

	BufferType* Get()
	{
		if (free_buffers.size() > 0)
		{
			BufferType* buff = free_buffers.back();
			buff->ptr = 0U;
			busy_buffers.push_back(buff);
			free_buffers.pop_back();
			return buff;
		}
		return nullptr;
	}

	bool Free(BufferType* buffer)
	{
		if (auto it = std::find(busy_buffers.begin(), busy_buffers.end(), buffer); it != busy_buffers.end())
		{
			(*it)->ptr = 0U;
			free_buffers.push_back(*it);
			busy_buffers.erase(it);
			return true;
		}
		return false;
	}

};