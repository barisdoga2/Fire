#pragma once

#include "FireNet.hpp"



class EasyBuffer;
class EasyPacket {
private:
    EasyBuffer* m_buffer;

public:
    EasyPacket(EasyBuffer* buffer);

    static bool MakeEncrypted(const CryptData& crypt, EasyBuffer* buffer);
    static bool MakeDecrypted(const CryptData& crypt, EasyBuffer* buffer);

    static bool MakeCompressed(EasyBuffer* in, EasyBuffer* out);
    static bool MakeDecompressed(EasyBuffer* in, EasyBuffer* out);

    SessionID_t* SessionID() const;

    SequenceID_t* SequenceID() const;

    IV_t* IV() const;

    uint8_t* Payload() const;

    size_t PayloadSize() const;

    static size_t HeaderSize();

    static size_t MinimumSize();

    // Static Accesssers
    static SequenceID_t* SequenceID(const EasyBuffer* buffer);

};
