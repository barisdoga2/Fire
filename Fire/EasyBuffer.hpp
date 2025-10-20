#pragma once

#include <iostream>
#include <vector>
#include <mutex>
#include <string>
#include <stdio.h>



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

	void reset()
	{
		m_payload_size = 0U;
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
	size_t gets = 0U, frees = 0U;
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

	EasyBuffer* Get(bool force = false)
	{
		EasyBuffer* buff = nullptr;
		if (force)
		{
			lock.lock();
			gets++;
			buff = free_buffers.back();
			buff->clear();
			buff->m_payload_size = 0U;
			busy_buffers.push_back(buff);
			free_buffers.pop_back();
			lock.unlock();
		}
		else
		{
			if (lock.try_lock())
			{
				if (busy_buffers.size() > 0)
				{
					gets++;
					buff = free_buffers.back();
					buff->clear();
					buff->m_payload_size = 0U;
					busy_buffers.push_back(buff);
					free_buffers.pop_back();
				}
				lock.unlock();
			}
		}
		return buff;
	}

	bool Free(EasyBuffer* buffer)
	{
		lock.lock();
		if (!buffer)
		{
			lock.unlock();
			return false;
		}
		bool ret = false;
		if (auto it = std::find(busy_buffers.begin(), busy_buffers.end(), buffer); it != busy_buffers.end())
		{
			frees++;
			(*it)->m_payload_size = 0U;
			free_buffers.push_back(*it);
			busy_buffers.erase(it);
			ret = true;
		}
		lock.unlock();
		return ret;
	}

	std::string Stats()
	{
		lock.lock();
		std::string ret;
		ret += "Buffer Manager Statistics:";
		ret += "    Total Buffers: " + std::to_string(free_buffers.size() + busy_buffers.size()) + "\nFree Buffers : " + std::to_string(free_buffers.size()) + "\nBusy Buffers : " + std::to_string(busy_buffers.size()) + "\nTotal Get : " + std::to_string(gets) + "\nTotal Free : " + std::to_string(frees) + "\n";
		lock.unlock();
		return ret;
	}

};