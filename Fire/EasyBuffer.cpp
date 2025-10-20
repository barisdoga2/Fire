#include "EasyBuffer.hpp"


EasyBuffer::EasyBuffer(size_t capacity) : m_capacity(capacity), m_payload_size(0U)
{
	m_begin = (uint8_t*)malloc(capacity);
	clear();
}

EasyBuffer::~EasyBuffer()
{
	free(m_begin);
}

void EasyBuffer::clear()
{
	memset(m_begin, 0, m_capacity);
}

void EasyBuffer::reset()
{
	m_payload_size = 0U;
}

uint8_t* EasyBuffer::begin() const
{
	return m_begin;
}

size_t EasyBuffer::capacity() const
{
	return m_capacity;
}

EasyBufferManager::EasyBufferManager(const size_t buffer_count, const size_t buffer_length) : buffer_count(buffer_count), buffer_length(buffer_length)
{
	for (size_t i = 0U; i < buffer_count; ++i)
	{
		EasyBuffer* buffer = new EasyBuffer(buffer_length);
		free_buffers.push_back(buffer);
	}
}

EasyBufferManager::~EasyBufferManager()
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

EasyBuffer* EasyBufferManager::Get(bool force)
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

bool EasyBufferManager::Free(EasyBuffer* buffer)
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

std::string EasyBufferManager::Stats()
{
	lock.lock();
	std::string ret;
	ret += "Buffer Manager Statistics:";
	ret += "    Total Buffers: " + std::to_string(free_buffers.size() + busy_buffers.size()) + "\nFree Buffers : " + std::to_string(free_buffers.size()) + "\nBusy Buffers : " + std::to_string(busy_buffers.size()) + "\nTotal Get : " + std::to_string(gets) + "\nTotal Free : " + std::to_string(frees) + "\n";
	lock.unlock();
	return ret;
}

