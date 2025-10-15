#include "EasyBuffer.hpp"

EasyBuffer::EasyBuffer() : vector(1472U), ptr(0U)
{
	
}

EasyBuffer::~EasyBuffer()
{

}

void EasyBuffer::Release()
{
	ptr = 0U;
	EasyBuffer::Free(this);
}

void* EasyBuffer::Data()
{
	return data();
}

size_t EasyBuffer::Capacity()
{
	return size();
}

void EasyBuffer::Init(size_t buffer_count)
{
	if (!isInit)
	{
		for (size_t i = 0; i < buffer_count; i++)
			freeBuffers.push_back(new EasyBuffer());
		isInit = true;
	}
}

void EasyBuffer::DeInit()
{
	if (isInit)
	{
		for (EasyBuffer* b : freeBuffers)
			delete b;
		for (EasyBuffer* b : busyBuffers)
			delete b;
		freeBuffers.clear();
		busyBuffers.clear();
		isInit = false;
	}
}

EasyBuffer* EasyBuffer::Get()
{
	EasyBuffer* b = nullptr;
	if (isInit && freeBuffers.size() > 0U)
	{
		b = freeBuffers.back();
		freeBuffers.pop_back();
		busyBuffers.push_back(b);
	}
	return b;
}

void EasyBuffer::Free(EasyBuffer* b)
{
	if (auto it = std::find(busyBuffers.begin(), busyBuffers.end(), b); it != busyBuffers.end())
	{
		freeBuffers.push_back(b);
		busyBuffers.erase(it);
	}
}