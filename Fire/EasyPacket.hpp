#pragma once

#include "EasyNet.hpp"
#include "EasyBuffer.hpp"
#include "EasyPeer.hpp"



class EasyPacket {
private:
    EasyBuffer* m_buffer;

public:
    EasyPacket(EasyBuffer* buffer);

    bool MakeEncrypted(EasyPeer& peer);

    bool MakeDecrypted(EasyPeer& peer);

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
        return m_buffer->begin() + sizeof(SessionID_t) + sizeof(SequenceID_t) + IV_SIZE;
    }

    inline size_t PayloadSize() const {
        return m_buffer->m_payload_size;
    }
};