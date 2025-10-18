#pragma once

#include <queue>
#include "EasyNet.hpp"
#include "EasyBuffer.hpp"
#include "EasyPeer.hpp"
#include "EasyCompression.hpp"



class EasyPacket {
private:
    EasyBuffer* m_buffer;

public:
    EasyPacket(EasyBuffer* buffer);

    bool MakeEncrypted(EasyPeer& peer);
    bool MakeDecrypted(EasyPeer& peer);

    bool MakeCompressed(EasyBuffer* out);
    bool MakeDecompressed(EasyBuffer* out);

    static bool MakeEncrypted(EasyPeer& peer, EasyBuffer* buffer);
    static bool MakeDecrypted(EasyPeer& peer, EasyBuffer* buffer);

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
};