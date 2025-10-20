#include "EasyBuffer.hpp"

#include "Net.hpp"
#include <sstream>

#ifdef SERVER_STATISTICS
#define STATS(x)                            stats_bufferManager.x
class Statistics {
public:
    StatisticsCounter_t gets{};
    StatisticsCounter_t getFails{};
    StatisticsCounter_t frees{};
    StatisticsCounter_t freeFails{};
};
Statistics stats_bufferManager;
#else
#define STATS(x)                            stats_bufferManager
StatisticsCounter_t stats_bufferManager{};
#endif
#define STATS_LOAD(x)                       (STATS(x).load())
#define STATS_INCREASE(x)                   (++STATS(x))
#define STATS_INCREASE_X(x, y)              (STATS(x).fetch_add(y))

#define STATS_GET                           STATS_INCREASE(gets)
#define STATS_GET_FAIL                      STATS_INCREASE(getFails)
#define STATS_FREE                          STATS_INCREASE(frees)
#define STATS_FREE_FAIL                     STATS_INCREASE(freeFails)

EasyBuffer::EasyBuffer(size_t capacity)
    : m_capacity(capacity),
    m_payload_size(0),
    m_data(std::make_unique<uint8_t[]>(capacity))
{
    clear();
}

EasyBuffer::~EasyBuffer() = default;

void EasyBuffer::clear() noexcept {
    std::memset(m_data.get(), 0, m_capacity);
    m_payload_size = 0;
}

void EasyBuffer::reset() noexcept {
    m_payload_size = 0;
}

uint8_t* EasyBuffer::begin() noexcept {
    return m_data.get();
}

const uint8_t* EasyBuffer::begin() const noexcept {
    return m_data.get();
}

size_t EasyBuffer::capacity() const noexcept {
    return m_capacity;
}

size_t EasyBuffer::size() const noexcept {
    return m_payload_size;
}

void EasyBuffer::setSize(size_t sz) noexcept {
    m_payload_size = (sz <= m_capacity) ? sz : m_capacity;
}

// ----------------------------------------------------------------------

EasyBufferManager::EasyBufferManager(size_t bufferCount, size_t bufferLength)
    : buffer_count(bufferCount), buffer_length(bufferLength)
{
    free_buffers.reserve(bufferCount);
    for (size_t i = 0; i < bufferCount; ++i)
        free_buffers.emplace_back(std::make_unique<EasyBuffer>(bufferLength));
}

EasyBufferManager::~EasyBufferManager()
{
    std::scoped_lock lock(mutex);
    for (auto* b : busy_buffers)
        delete b;
    busy_buffers.clear();
}

EasyBuffer* EasyBufferManager::Get(bool force)
{
    std::scoped_lock lock(mutex);

    if (free_buffers.empty()) {
        STATS_GET_FAIL;
        return nullptr;
    }

    auto buffer = free_buffers.back().release();
    free_buffers.pop_back();
    busy_buffers.push_back(buffer);
    buffer->reset();

    STATS_GET;
    return buffer;
}

bool EasyBufferManager::Free(EasyBuffer* buffer)
{
    if (!buffer) {
        STATS_FREE_FAIL;
        return false;
    }

    std::scoped_lock lock(mutex);

    auto it = std::find(busy_buffers.begin(), busy_buffers.end(), buffer);
    if (it == busy_buffers.end()) {
        STATS_FREE_FAIL;
        return false;
    }

    buffer->reset();
    free_buffers.emplace_back(*it);
    busy_buffers.erase(it);

    STATS_FREE;
    return true;
}

std::string EasyBufferManager::Stats() const
{
#ifdef SERVER_STATISTICS
    std::ostringstream ss;
    ss << "========= Buffer Manager Statistics ==========\n";
    ss << "    Gets:             " << STATS_LOAD(gets) << "\n";
    ss << "    Get Fails:        " << STATS_LOAD(getFails) << "\n";
    ss << "    Frees:            " << STATS_LOAD(frees) << "\n";
    ss << "    Free Fails:       " << STATS_LOAD(freeFails) << "\n";
    ss << "    Free Count:       " << free_buffers.size() << "\n";
    ss << "    Busy Count:       " << busy_buffers.size() << "\n";
    ss << "==============================================\n";
    return ss.str();
#else
    return "Server statistics disabled.\n";
#endif
}
