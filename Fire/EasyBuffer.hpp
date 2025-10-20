#pragma once

#include <vector>
#include <mutex>
#include <algorithm>
#include <string>
#include <memory>
#include <cstring>

class EasyBuffer;
class EasyBufferManager;

class EasyBuffer {
public:
    explicit EasyBuffer(size_t capacity);
    ~EasyBuffer();

    void clear() noexcept;
    void reset() noexcept;

    [[nodiscard]] uint8_t* begin() noexcept;
    [[nodiscard]] const uint8_t* begin() const noexcept;
    [[nodiscard]] size_t capacity() const noexcept;
    [[nodiscard]] size_t size() const noexcept;

    void setSize(size_t sz) noexcept;

    /**/
    size_t m_payload_size;
    /**/
private:
    const size_t m_capacity;
    std::unique_ptr<uint8_t[]> m_data;

    friend class EasyBufferManager;
};

class EasyBufferManager {
public:
    EasyBufferManager(size_t bufferCount, size_t bufferLength);
    ~EasyBufferManager();

    EasyBuffer* Get(bool force = false);
    bool Free(EasyBuffer* buffer);
    [[nodiscard]] std::string Stats() const;

private:
    const size_t buffer_count;
    const size_t buffer_length;

    mutable std::mutex mutex;
    std::vector<std::unique_ptr<EasyBuffer>> free_buffers;
    std::vector<EasyBuffer*> busy_buffers;
};
