#pragma once

#include <vector>
#include <mutex>
#include <string>



class EasyBuffer {
private:
	const size_t m_capacity;
	uint8_t* m_begin;

public:
	size_t m_payload_size;

	EasyBuffer(size_t capacity);

	~EasyBuffer();

	void clear();

	void reset();

	uint8_t* begin() const;

	size_t capacity() const;
};

class EasyBufferManager {
public:
	using BufferPool = std::vector<EasyBuffer*>;
	const size_t buffer_count, buffer_length;
	BufferPool free_buffers;
	BufferPool busy_buffers;
	size_t gets = 0U, frees = 0U, get_fails = 0U, free_fails = 0U;
	std::mutex lock;

	EasyBufferManager() = delete;

	EasyBufferManager(const size_t buffer_count, const size_t buffer_length);

	~EasyBufferManager();

	EasyBuffer* Get(bool force = false);

	bool Free(EasyBuffer* buffer);

	std::string Stats();

};