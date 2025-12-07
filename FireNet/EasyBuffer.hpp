#pragma once

#include <vector>
#include <memory>
#include <string>
#include <mutex>



class EasyBufferManager;
class EasyBuffer {
public:
    explicit EasyBuffer(size_t capacity);
    ~EasyBuffer();

    const size_t m_capacity;
    uint8_t* m_data;

    size_t m_payload_size;

    void clear() noexcept;
    void reset() noexcept;

    [[nodiscard]] uint8_t* begin() noexcept;
    [[nodiscard]] const uint8_t* begin() const noexcept;
    [[nodiscard]] size_t capacity() const noexcept;
    [[nodiscard]] size_t size() const noexcept;

};

class EasyBufferManager {
    std::mutex m;
    const size_t buffer_count;
    const size_t buffer_length;

    std::vector<EasyBuffer*> free_buffers;
    std::vector<EasyBuffer*> busy_buffers;

public:
    EasyBufferManager(size_t bufferCount = 50U, size_t bufferLength = 1472U);
    ~EasyBufferManager();

    EasyBuffer* Get();
    bool Free(EasyBuffer* buffer);
    [[nodiscard]] std::string Stats() const;

};
