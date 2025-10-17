#pragma once

#include <iostream>
#include <vector>
#include <mutex>



class EasyBuffer {
private:
	const size_t m_capacity;
	uint8_t* m_begin;

public:
	size_t m_payload_size;

	EasyBuffer(size_t capacity) : m_capacity(capacity), m_payload_size(0U)
	{
		m_begin = (uint8_t*)malloc(capacity);
		clear();
	}

	~EasyBuffer()
	{
		free(m_begin);
	}

	void clear()
	{
		memset(m_begin, 0, m_capacity);
	}

	uint8_t* begin() const
	{
		return m_begin;
	}

	size_t capacity() const
	{
		return m_capacity;
	}
};

class EasyBufferManager {
public:
	using BufferPool = std::vector<EasyBuffer*>;
	const size_t buffer_count, buffer_length;
	BufferPool free_buffers;
	BufferPool busy_buffers;
	std::mutex lock;

	EasyBufferManager() = delete;

	EasyBufferManager(const size_t buffer_count, const size_t buffer_length) : buffer_count(buffer_count), buffer_length(buffer_length)
	{
		for (size_t i = 0U; i < buffer_count; ++i)
		{
			EasyBuffer* buffer = new EasyBuffer(buffer_length);
			free_buffers.push_back(buffer);
		}
	}

	~EasyBufferManager()
	{
		lock.lock();
		for (auto b : free_buffers)
			delete b;
		for (auto b : busy_buffers)
			delete b;
		free_buffers.clear();
		busy_buffers.clear();
		lock.unlock();
	}

	EasyBuffer* Get()
	{
		EasyBuffer* buff = nullptr;
		if (lock.try_lock())
		{
			if (free_buffers.size() > 0)
			{
				buff = free_buffers.back();
				buff->clear();
				buff->m_payload_size = 0U;
				busy_buffers.push_back(buff);
				free_buffers.pop_back();
			}
			lock.unlock();
		}
		return buff;
	}

	bool Free(EasyBuffer* buffer)
	{
		if (!buffer)
			return false;
		bool ret = false;
		lock.lock();
		if (auto it = std::find(busy_buffers.begin(), busy_buffers.end(), buffer); it != busy_buffers.end())
		{
			(*it)->m_payload_size = 0U;
			free_buffers.push_back(*it);
			busy_buffers.erase(it);
			ret = true;
		}
		lock.unlock();
		return ret;
	}

};