#pragma once

#include "EasyNet.hpp"
#include "EasyBuffer.hpp"


class EasyBuffer;
class EasyPacket {
private:
    EasyBuffer* m_buffer;

public:
    EasyPacket(EasyBuffer* buffer);

    static bool MakeEncrypted(const PeerCryptInfo& crypt, EasyBuffer* buffer);
    static bool MakeDecrypted(const PeerCryptInfo& crypt, EasyBuffer* buffer);

    static bool MakeCompressed(EasyBuffer* in, EasyBuffer* out);
    static bool MakeDecompressed(EasyBuffer* in, EasyBuffer* out);

    inline SessionID_t* SessionID() const {
        return (SessionID_t*)(m_buffer->begin());
    }

    inline SequenceID_t* SequenceID() const {
        return (SequenceID_t*)(m_buffer->begin() + sizeof(SessionID_t));
    }

    inline IV_t* IV() const {
        return (IV_t*)(m_buffer->begin() + sizeof(SessionID_t) + sizeof(SequenceID_t));
    }

    inline uint8_t* Payload() const {
        return m_buffer->begin() + HeaderSize();
    }

    inline size_t PayloadSize() const {
        return m_buffer->m_payload_size;
    }

    static inline size_t HeaderSize() {
        return sizeof(SessionID_t) + sizeof(SequenceID_t) + IV_SIZE;
    }

    static inline size_t MinimumSize() {
        return sizeof(SessionID_t) + sizeof(SequenceID_t) + IV_SIZE + 1U + TAG_SIZE;
    }
};